/*************************************************************************/
/*  ewbik_segmented_skeleton_3d.cpp                                      */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "ewbik_segmented_skeleton_3d.h"

Ref<IKBone> IKBoneChain::get_root() const {
	return root;
}
Ref<IKBone> IKBoneChain::get_tip() const {
	return tip;
}

int32_t IKBoneChain::get_chain_length() const {
	return chain_length;
}

bool IKBoneChain::is_root_effector() const {
	return root->is_effector();
}

bool IKBoneChain::is_tip_effector() const {
	return tip->is_effector();
}

Vector<Ref<IKBoneChain>> IKBoneChain::get_effector_direct_descendents() const {
	return effector_direct_descendents;
}

int32_t IKBoneChain::get_effector_direct_descendents_size() const {
	return effector_direct_descendents.size();
}

BoneId IKBoneChain::find_root_bone_id(BoneId p_bone) {
	BoneId root_id = p_bone;
	while (skeleton->get_bone_parent(root_id) != -1) {
		root_id = skeleton->get_bone_parent(root_id);
	}

	return root_id;
}

void IKBoneChain::generate_skeleton_segments(const HashMap<BoneId, Ref<IKBone>> &p_map) {
	child_chains.clear();

	Ref<IKBone> tempTip = root;
	chain_length = 0;
	while (true) {
		chain_length++;
		Vector<BoneId> children_with_effector_descendants = tempTip->get_children_with_effector_descendants(skeleton, p_map);
		if (children_with_effector_descendants.size() > 1 || tempTip->is_effector()) {
			tip = tempTip;
			for (int32_t child_i = 0; child_i < children_with_effector_descendants.size(); child_i++) {
				BoneId child_bone = children_with_effector_descendants[child_i];
				child_chains.push_back(Ref<IKBoneChain>(memnew(IKBoneChain(skeleton, child_bone, p_map, tip))));
			}
			break;
		} else if (children_with_effector_descendants.size() == 1) {
			BoneId bone_id = children_with_effector_descendants[0];
			if (p_map.has(bone_id)) {
				Ref<IKBone> next = p_map[bone_id];
				next->set_parent(tempTip);
				tempTip = next;
			} else {
				Ref<IKBone> next = Ref<IKBone>(memnew(IKBone(bone_id, tempTip)));
				tempTip = next;
			}
		} else {
			tip = tempTip;
			break;
		}
	}
	update_segmented_skeleton();
}

void IKBoneChain::update_segmented_skeleton() {
	update_effector_direct_descendents();
	generate_bones_map();
}

void IKBoneChain::update_effector_direct_descendents() {
	effector_direct_descendents.clear();
	if (is_tip_effector()) {
		effector_direct_descendents.push_back(this);
	} else {
		for (int32_t child_i = 0; child_i < child_chains.size(); child_i++) {
			Ref<IKBoneChain> child_segment = child_chains[child_i];
			effector_direct_descendents.append_array(child_segment->get_effector_direct_descendents());
		}
	}
}

void IKBoneChain::generate_bones_map() {
	bones_map.clear();
	Ref<IKBone> current_bone = tip;
	Ref<IKBone> stop_on = root;
	while (current_bone.is_valid()) {
		bones_map[current_bone->get_bone_id()] = current_bone;
		if (current_bone == stop_on)
			break;
		current_bone = current_bone->get_parent();
	}
}

void IKBoneChain::generate_default_segments_from_root() {
	child_chains.clear();

	Ref<IKBone> tempTip = root;
	chain_length = 0;
	while (true) {
		chain_length++;
		Vector<BoneId> children = skeleton->get_bone_children(tempTip->get_bone_id());
		if (children.size() > 1) {
			tip = tempTip;
			for (int32_t child_i = 0; child_i < children.size(); child_i++) {
				BoneId child_bone = children[child_i];
				Ref<IKBoneChain> child_segment = Ref<IKBoneChain>(memnew(IKBoneChain(skeleton, child_bone, tip)));
				child_segment->generate_default_segments_from_root();
				child_chains.push_back(child_segment);
			}
			break;
		} else if (children.size() == 1) {
			BoneId bone_id = children[0];
			Ref<IKBone> next = Ref<IKBone>(memnew(IKBone(bone_id, tempTip)));
			tempTip = next;
		} else {
			tip = tempTip;
			tip->create_effector();
			break;
		}
	}
	update_segmented_skeleton();
}

Ref<IKBoneChain> IKBoneChain::get_child_segment_containing(const Ref<IKBone> &p_bone) {
	if (bones_map.has(p_bone->get_bone_id())) {
		return this;
	} else {
		for (int32_t child_i = 0; child_i < child_chains.size(); child_i++) {
			Ref<IKBoneChain> child_segment = child_chains.write[child_i]->get_child_segment_containing(p_bone);
			if (child_segment.is_valid())
				return child_segment;
		}
	}
	return nullptr;
}

void IKBoneChain::get_bone_list(Vector<Ref<IKBone>> &p_list) const {
	for (int32_t child_i = 0; child_i < child_chains.size(); child_i++) {
		child_chains[child_i]->get_bone_list(p_list);
	}
	Ref<IKBone> current_bone = tip;
	while (current_bone.is_valid()) {
		p_list.push_back(current_bone);
		if (current_bone == root)
			break;
		current_bone = current_bone->get_parent();
	}
}

void IKBoneChain::update_effector_list(Vector<Ref<EWBIKBoneEffector3D>> &p_list) {
	idx_eff_i = p_list.size();
	for (int32_t chain_i = 0; chain_i < child_chains.size(); chain_i++) {
		Ref<IKBoneChain> chain = child_chains[chain_i];
		chain->update_effector_list(p_list);
	}
	if (is_tip_effector()) {
		p_list.push_back(tip->get_effector());
	}
	idx_eff_f = p_list.size();
}

void IKBoneChain::update_optimal_rotation(Ref<IKBone> p_for_bone, IKState &p_state, bool p_translate, int32_t p_stabilization_passes) {
	Quat best_orientation = p_for_bone->get_global_transform().basis.get_quat();
	real_t new_dampening = p_translate ? Math_PI : -1.0;
	if (p_for_bone->get_parent().is_null() || (child_chains.is_empty() && tip->get_effector()->is_following_translation_only())) {
		p_stabilization_passes = 0;
	}

	update_tip_headings(p_state);

	real_t best_rmsd = 0.0;
	real_t new_rmsd = FLT_MAX;

	if (p_stabilization_passes > 0)
		best_rmsd = get_manual_rmsd(p_state);

	for (int32_t i = 0; i < p_stabilization_passes; i++) {
		set_optimal_rotation(p_for_bone, p_state);
	}
}

void IKBoneChain::set_optimal_rotation(Ref<IKBone> p_for_bone, IKState &p_state) {
	QCP qcp;
	Vector3 translation;
	Quat rot = qcp.calc_optimal_rotation(p_state.tip_headings, p_state.target_headings, p_state.heading_weights, true, translation);
	p_for_bone->rotate(rot);
}

real_t IKBoneChain::get_manual_rmsd(const IKState &p_state) const {
	real_t rmsd = 0.0;
	real_t wsum = 0.0;
	for (int32_t i = idx_headings_i; i < idx_headings_f; i++) {
		Vector3 tiph = p_state.tip_headings[i];
		Vector3 targeth = p_state.target_headings[i];
		real_t xd = targeth.x - tiph.x;
		real_t yd = targeth.y - tiph.y;
		real_t zd = targeth.z - tiph.z;
		real_t w = p_state.heading_weights[i];
		rmsd += w * (xd * xd + yd * yd + zd * zd);
		wsum += w;
	}

	rmsd /= wsum;
	return rmsd;
}

void IKBoneChain::update_target_headings(IKState &p_state) {
	idx_headings_i = p_state.heading_weights.size();
	for (int32_t effector_i = idx_eff_i; effector_i < idx_eff_f; effector_i++) {
		Ref<EWBIKBoneEffector3D> effector = p_state.ordered_effector_list[effector_i];
		effector->update_target_headings(skeleton, p_state.target_headings, p_state.heading_weights);
	}
	idx_headings_f = p_state.heading_weights.size();
}

void IKBoneChain::update_tip_headings(IKState &p_state) {
	int32_t index = idx_headings_i; // Index is incremented by update_tip_headings function
	for (int32_t effector_i = idx_eff_i; effector_i < idx_eff_f; effector_i++) {
		Ref<EWBIKBoneEffector3D> effector = p_state.ordered_effector_list[effector_i];
		effector->update_tip_headings(skeleton, p_state.tip_headings, index);
	}
}

void IKBoneChain::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_root_effector"), &IKBoneChain::is_root_effector);
	ClassDB::bind_method(D_METHOD("is_tip_effector"), &IKBoneChain::is_tip_effector);
}

IKBoneChain::IKBoneChain(Skeleton3D *p_skeleton, BoneId p_root_bone, const Ref<IKBoneChain> &p_parent) {
	skeleton = p_skeleton;
	BoneId root_id = find_root_bone_id(p_root_bone);
	root = Ref<IKBone>(memnew(IKBone(root_id)));
	if (p_parent.is_valid()) {
		parent_chain = p_parent;
		root->set_parent(p_parent->get_tip());
	}
}

IKBoneChain::IKBoneChain(Skeleton3D *p_skeleton, BoneId p_root_bone,
		const HashMap<BoneId, Ref<IKBone>> &p_map, const Ref<IKBoneChain> &p_parent) {
	skeleton = p_skeleton;
	BoneId root_id = find_root_bone_id(p_root_bone);
	if (p_map.has(root_id)) {
		root = p_map[root_id];
	} else {
		root = Ref<IKBone>(memnew(IKBone(root_id)));
	}
	if (p_parent.is_valid()) {
		parent_chain = p_parent;
		root->set_parent(p_parent->get_tip());
	}
	generate_skeleton_segments(p_map);
}