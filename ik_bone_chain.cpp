/*************************************************************************/
/*  ik_bone_chain.cpp                                      */
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

#include "ik_bone_chain.h"

Ref<IKBone3D> IKBoneChain::get_root() const {
	return root;
}
Ref<IKBone3D> IKBoneChain::get_tip() const {
	return tip;
}

bool IKBoneChain::is_root_pinned() const {
	return root->get_parent().is_valid() && root->get_parent()->is_effector();
}

bool IKBoneChain::is_tip_effector() const {
	return tip->is_effector();
}

Vector<Ref<IKBoneChain>> IKBoneChain::get_child_chains() const {
	return child_chains;
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

void IKBoneChain::generate_skeleton_segments(const HashMap<BoneId, Ref<IKBone3D>> &p_map) {
	child_chains.clear();

	Ref<IKBone3D> tempTip = root;
	while (true) {
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
				Ref<IKBone3D> next = p_map[bone_id];
				next->set_parent(tempTip);
				tempTip = next;
			} else {
				Ref<IKBone3D> next = Ref<IKBone3D>(memnew(IKBone3D(bone_id, tempTip)));
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
	Ref<IKBone3D> current_bone = tip;
	Ref<IKBone3D> stop_on = root;
	while (current_bone.is_valid()) {
		bones_map[current_bone->get_bone_id()] = current_bone;
		if (current_bone == stop_on) {
			break;
		}
		current_bone = current_bone->get_parent();
	}
}

void IKBoneChain::generate_default_segments_from_root() {
	child_chains.clear();

	Ref<IKBone3D> tempTip = root;
	while (true) {
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
			Ref<IKBone3D> next = Ref<IKBone3D>(memnew(IKBone3D(bone_id, tempTip)));
			tempTip = next;
		} else {
			tip = tempTip;
			tip->create_effector();
			break;
		}
	}
	update_segmented_skeleton();
}

Ref<IKBoneChain> IKBoneChain::get_child_segment_containing(const Ref<IKBone3D> &p_bone) {
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

void IKBoneChain::get_bone_list(Vector<Ref<IKBone3D>> &p_list) const {
	for (int32_t child_i = 0; child_i < child_chains.size(); child_i++) {
		child_chains[child_i]->get_bone_list(p_list);
	}
	Ref<IKBone3D> current_bone = tip;
	while (current_bone.is_valid()) {
		p_list.push_back(current_bone);
		if (current_bone == root)
			break;
		current_bone = current_bone->get_parent();
	}
}

void IKBoneChain::update_effector_list() {
	heading_weights.clear();
	real_t depth_falloff = is_tip_effector() ? tip->get_effector()->depth_falloff : 1.0;
	for (int32_t chain_i = 0; chain_i < child_chains.size(); chain_i++) {
		Ref<IKBoneChain> chain = child_chains[chain_i];
		chain->update_effector_list();
		if (depth_falloff > CMP_EPSILON) {
			effector_list.append_array(chain->effector_list);
			for (int32_t w_i = 0; w_i < chain->heading_weights.size(); w_i++) {
				heading_weights.push_back(chain->heading_weights[w_i] * depth_falloff);
			}
		}
	}
	if (is_tip_effector()) {
		Ref<IKEffector3D> effector = tip->get_effector();
		effector_list.push_back(effector);
		heading_weights.push_back(effector->weight);
		heading_weights.push_back(effector->weight);
	}
	create_headings();
}

void IKBoneChain::update_optimal_rotation(Ref<IKBone3D> p_for_bone, int32_t p_constraint_stabilization_passes) {
	if (p_constraint_stabilization_passes == 0) {
		return;
	}
	Vector<real_t> *weights = nullptr;
	Vector<Vector3> *htarget = update_target_headings(p_for_bone, weights);
	Vector<Vector3> *htip = update_tip_headings(p_for_bone);
	if (p_for_bone->get_parent().is_null() || htarget->size() == 1) {
		p_constraint_stabilization_passes = 0;
	}

	real_t best_sqrmsd = 0.0;

	if (p_constraint_stabilization_passes > 0) {
		best_sqrmsd = get_manual_sqrtmsd(*htarget, *htip, *weights);
	}

	real_t sqrmsd = FLT_MAX;
	for (int32_t i = 0; i < p_constraint_stabilization_passes + 1; i++) {
		if (p_constraint_stabilization_passes <= 0) {
			break;
		}
		sqrmsd = set_optimal_rotation(p_for_bone, *htarget, *htip, *weights);
		if (sqrmsd <= best_sqrmsd) {
			best_sqrmsd = sqrmsd;
		}
		float sqrmsd_snapped = Math::stepify(sqrmsd, CMP_EPSILON);
		float best_sqrmsd_snapped = Math::stepify(best_sqrmsd, CMP_EPSILON);
		if (Math::is_equal_approx(sqrmsd_snapped, best_sqrmsd_snapped)) {
			break;
		}
	}
}

Quat IKBoneChain::set_quadrance_angle(Quat p_quat, real_t p_cos_half_angle) const {
	float squared_sine = p_quat.x * p_quat.x + p_quat.y * p_quat.y + p_quat.z * p_quat.z;
	Quat rot = p_quat;
	if (!Math::is_zero_approx(squared_sine)) {
		float inverse_coeff = Math::sqrt(((1.0f - (p_cos_half_angle * p_cos_half_angle)) / squared_sine));
		rot.x = inverse_coeff * p_quat.x;
		rot.y = inverse_coeff * p_quat.y;
		rot.z = inverse_coeff * p_quat.z;
		rot.w = p_quat.w < 0 ? -p_cos_half_angle : p_cos_half_angle;
	}
	return rot;
}

Quat IKBoneChain::clamp_to_angle(Quat p_quat, real_t p_angle) const {
	float cos_half_angle = Math::cos(0.5f * p_angle);
	return clamp_to_quadrance_angle(p_quat, cos_half_angle);
}

Quat IKBoneChain::clamp_to_quadrance_angle(Quat p_quat, real_t p_cos_half_angle) const {
	float new_coeff = 1.0f - (p_cos_half_angle * p_cos_half_angle);
	float current_coeff = p_quat.x * p_quat.x + p_quat.y * p_quat.y + p_quat.z * p_quat.z;
	Quat rot = p_quat;
	if (new_coeff > current_coeff) {
		return rot;
	} else {
		float compositeCoeff = Math::sqrt(new_coeff / current_coeff);
		rot.x *= compositeCoeff;
		rot.y *= compositeCoeff;
		rot.z *= compositeCoeff;
		rot.w = p_quat.w < 0 ? -p_cos_half_angle : p_cos_half_angle;
	}
	return rot;
}

real_t IKBoneChain::set_optimal_rotation(Ref<IKBone3D> p_for_bone, const Vector<Vector3> &p_htarget,
		const Vector<Vector3> &p_htip, const Vector<real_t> &p_weights, float p_dampening) {
	Quat rot;
	real_t sqrmsd = qcp.calc_optimal_rotation(p_htip, p_htarget, p_weights, rot);

	float bone_damp = p_for_bone->get_cos_half_dampen();

	if (!Math::is_equal_approx(p_dampening, -1.0)) {
		bone_damp = p_dampening;
		rot = clamp_to_angle(rot, bone_damp);
	} else {
		rot = clamp_to_quadrance_angle(rot, bone_damp);
	}

	p_for_bone->set_rot_delta(rot);
	return sqrmsd;
}

void IKBoneChain::create_headings() {
	target_headings.clear();
	tip_headings.clear();
	int32_t n = heading_weights.size();
	target_headings.resize(n);
	tip_headings.resize(n);

	if (is_tip_effector()) {
		tip->get_effector()->create_headings(heading_weights);
	}
}

Vector<Vector3> *IKBoneChain::update_target_headings(Ref<IKBone3D> p_for_bone, Vector<real_t> *&p_weights) {
	Vector<Vector3> *htarget;
	if (tip == p_for_bone && tip->is_effector()) {
		Ref<IKEffector3D> effector = tip->get_effector();
		htarget = &effector->target_headings;
		p_weights = &effector->heading_weights;
	} else {
		htarget = &target_headings;
		p_weights = &heading_weights;
	}
	int32_t index = 0; // Index is increased by effector->update_target_headings() function
	for (int32_t effector_i = 0; effector_i < effector_list.size(); effector_i++) {
		Ref<IKEffector3D> effector = effector_list[effector_i];
		effector->update_target_headings(p_for_bone, htarget, index, p_weights);
	}
	return htarget;
}

Vector<Vector3> *IKBoneChain::update_tip_headings(Ref<IKBone3D> p_for_bone) {
	Vector<Vector3> *htip;
	if (tip == p_for_bone && tip->is_effector()) {
		Ref<IKEffector3D> effector = tip->get_effector();
		htip = &effector->tip_headings;
	} else {
		htip = &tip_headings;
	}
	int32_t index = 0; // Index is increased by effector->update_target_headings() function
	for (int32_t effector_i = 0; effector_i < effector_list.size(); effector_i++) {
		Ref<IKEffector3D> effector = effector_list[effector_i];
		effector->update_tip_headings(p_for_bone, htip, index);
	}
	return htip;
}

void IKBoneChain::grouped_segment_solver(int32_t p_constraint_stabilization_passes) {
	segment_solver(p_constraint_stabilization_passes);
	for (int32_t i = 0; i < effector_direct_descendents.size(); i++) {
		Ref<IKBoneChain> effector_chain = effector_direct_descendents[i];
		for (int32_t child_i = 0; child_i < effector_chain->child_chains.size(); child_i++) {
			Ref<IKBoneChain> child = effector_chain->child_chains[child_i];
			child->grouped_segment_solver(p_constraint_stabilization_passes);
		}
	}
}

void IKBoneChain::segment_solver(int32_t p_constraint_stabilization_passes) {
	if (child_chains.size() == 0 && !is_tip_effector()) {
		return;
	} else if (!is_tip_effector()) {
		for (int32_t child_i = 0; child_i < child_chains.size(); child_i++) {
			Ref<IKBoneChain> child = child_chains[child_i];
			child->segment_solver(p_constraint_stabilization_passes);
		}
	}
	qcp_solver(p_constraint_stabilization_passes);
}

void IKBoneChain::qcp_solver(int32_t p_constraint_stabilization_passes) {
	Vector<Ref<IKBone3D>> list;
	get_bone_list(list);
	for (int32_t bone_i = 0; bone_i < list.size(); bone_i++) {
		Ref<IKBone3D> current_bone = list[bone_i];
		if (!current_bone->get_orientation_lock()) {
			update_optimal_rotation(current_bone, p_constraint_stabilization_passes);
		}
		if (current_bone == root) {
			break;
		}
	}
}

void IKBoneChain::debug_print_chains(Vector<bool> p_levels) {
	Vector<Ref<IKBone3D>> bone_list;
	Ref<IKBone3D> current_bone = tip;
	while (current_bone.is_valid()) {
		bone_list.push_back(current_bone);
		if (current_bone == root) {
			break;
		}
		current_bone = current_bone->get_parent();
	}
	String tab = "";
	for (int32_t lvl_i = 0; lvl_i < p_levels.size(); lvl_i++) {
		if (p_levels[lvl_i]) {
			tab += "  |";
		} else {
			tab += "  ";
		}
	}
	String t = "";
	if (p_levels.size() == 0 || !p_levels[p_levels.size() - 1]) {
		t = "|";
	}
	for (int32_t b_i = bone_list.size() - 1; b_i > -1; b_i--) {
		String s = tab + t + "_";
		Ref<IKBone3D> bone = bone_list[b_i];
		if (bone == root && bone == tip) {
			if (tip->is_effector()) {
				s += "(RTE) ";
			} else {
				s += "(RT) ";
			}
		} else if (bone == root) {
			s += "(R) ";
		} else if (bone == tip) {
			if (tip->is_effector()) {
				s += "(TE) ";
			} else {
				s += "(T) ";
			}
		}
		s += skeleton->get_bone_name(bone->get_bone_id());
		print_line(s);
	}
	for (int32_t chain_i = 0; chain_i < child_chains.size(); chain_i++) {
		Vector<bool> levels = p_levels;
		levels.push_back(chain_i != child_chains.size() - 1);
		Ref<IKBoneChain> chain = child_chains[chain_i];
		chain->debug_print_chains(levels);
		if (chain_i < child_chains.size() - 1) {
			print_line(tab + "  |");
		}
	}
}

void IKBoneChain::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_root_pinned"), &IKBoneChain::is_root_pinned);
	ClassDB::bind_method(D_METHOD("is_tip_effector"), &IKBoneChain::is_tip_effector);
}

IKBoneChain::IKBoneChain(Skeleton *p_skeleton, BoneId p_root_bone, const Ref<IKBoneChain> &p_parent) {
	skeleton = p_skeleton;
	root = Ref<IKBone3D>(memnew(IKBone3D(p_root_bone)));
	if (p_parent.is_valid()) {
		parent_chain = p_parent;
		root->set_parent(p_parent->get_tip());
	}
}

IKBoneChain::IKBoneChain(Skeleton *p_skeleton, BoneId p_root_bone,
		const HashMap<BoneId, Ref<IKBone3D>> &p_map, const Ref<IKBoneChain> &p_parent) {
	skeleton = p_skeleton;
	if (p_map.has(p_root_bone)) {
		root = p_map[p_root_bone];
	} else {
		root = Ref<IKBone3D>(memnew(IKBone3D(p_root_bone)));
	}
	if (p_parent.is_valid()) {
		parent_chain = p_parent;
		root->set_parent(p_parent->get_tip());
	}
	generate_skeleton_segments(p_map);
}

real_t IKBoneChain::get_manual_sqrtmsd(const Vector<Vector3> &p_htarget, const Vector<Vector3> &p_htip, const Vector<real_t> &p_weights) const {
	double manual_sqrtmsd = 0.0;
	double w_sum = 0.0;
	for (int i = 0; i < p_htarget.size(); i++) {
		real_t distance_sq = p_htarget[i].distance_squared_to(p_htip[i]);
		double mag_sq = p_weights[i] * distance_sq;
		manual_sqrtmsd += mag_sq;
		w_sum += p_weights[i];
	}
	manual_sqrtmsd /= w_sum;

	return manual_sqrtmsd;
}