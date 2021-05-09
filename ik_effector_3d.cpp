/*************************************************************************/
/*  ik_effector_3d.cpp                                           */
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

#include "ik_effector_3d.h"

void IKEffector3D::set_target_transform(const Transform &p_target_transform) {
	target_transform = p_target_transform;
}

Transform IKEffector3D::get_target_transform() const {
	return target_transform;
}

void IKEffector3D::set_target_node(const NodePath &p_target_node_path) {
	target_nodepath = p_target_node_path;
}

NodePath IKEffector3D::get_target_node() const {
	return target_nodepath;
}

void IKEffector3D::set_use_target_node_rotation(bool p_use) {
	use_target_node_rotation = p_use;
}

bool IKEffector3D::get_use_target_node_rotation() const {
	return use_target_node_rotation;
}

Transform IKEffector3D::get_goal_transform() const {
	return goal_transform;
}

bool IKEffector3D::is_node_xform_changed(Skeleton *p_skeleton) const {
	Node *node = p_skeleton->get_node_or_null(target_nodepath);
	if (node && node->is_class("Spatial")) {
		Spatial *target_node = Object::cast_to<Spatial>(node);
		return prev_node_xform != target_node->get_global_transform();
	}
	return false;
}

Ref<IKBone3D> IKEffector3D::get_shadow_bone() const {
	return for_bone;
}

bool IKEffector3D::is_following_translation_only() const {
	return !(follow_x || follow_y || follow_z);
}

void IKEffector3D::update_goal_transform(Skeleton *p_skeleton) {
	goal_transform = Transform();
	Node *node = p_skeleton->get_node_or_null(target_nodepath);
	if (node && node->is_class("Spatial")) {
		Spatial *target_node = Object::cast_to<Spatial>(node);
		Transform node_xform = target_node->get_global_transform();
		if (use_target_node_rotation) {
			goal_transform = p_skeleton->get_global_transform().affine_inverse() * node_xform;
		} else {
			goal_transform = Transform(Basis(), p_skeleton->to_local(node_xform.origin));
		}
		prev_node_xform = node_xform;
		goal_transform = target_transform * goal_transform;
	} else {
		goal_transform = for_bone->get_global_transform() * target_transform;
	}
}

void IKEffector3D::update_priorities() {
	follow_x = priority.x > 0.0;
	follow_y = priority.y > 0.0;
	follow_z = priority.z > 0.0;

	num_headings = 2;
	// if (follow_x) {
	// 	num_headings += 2;
	// }
	// if (follow_y) {
	// 	num_headings += 2;
	// }
	// if (follow_z) {
	// 	num_headings += 2;
	// }
}

void IKEffector3D::create_headings(const Vector<real_t> &p_weights) {
	/**
	 * Weights are given from the parent chain. The last two weights should
	 * always correspond to this effector weights. In the parent only the origin
	 * is considered for rotation, but here the last two headings must be replaced
	 * by the corresponding number of "axis-orientation" headings.
	*/
	int32_t nw = p_weights.size() - 2;
	int32_t nheadings = nw + num_headings;
	heading_weights.resize(nheadings);
	tip_headings.resize(nheadings);
	target_headings.resize(nheadings);

	for (int32_t i_w = 0; i_w < nw; i_w++) {
		heading_weights.write[i_w] = p_weights[i_w];
	}

	int32_t index = 0;
	heading_weights.write[nw + index] = weight;
	heading_weights.write[nw + index + 1] = weight;
	index += 2;
	// index++;

	// if (follow_x) {
	// 	heading_weights.write[nw+index] = weight * priority.x;
	// 	heading_weights.write[nw+index+1] = weight * priority.x;
	// 	index += 2;
	// }

	// if (follow_y) {
	// 	heading_weights.write[nw+index] = weight * priority.y;
	// 	heading_weights.write[nw+index+1] = weight * priority.y;
	// 	index += 2;
	// }

	// if (follow_z) {
	// 	heading_weights.write[nw+index] = weight * priority.z;
	// 	heading_weights.write[nw+index+1] = weight * priority.z;
	// }
}

void IKEffector3D::update_target_headings(Ref<IKBone3D> p_for_bone, Vector<Vector3> *p_headings, int32_t &p_index,
		Vector<real_t> *p_weights) const {
	Vector3 origin = p_for_bone->get_global_transform().origin;
	if (p_for_bone == for_bone) {
		/**
		 * The following block corresponds to the original implementation
		*/
		// p_headings->write[p_index] = goal_transform.origin - origin;
		// p_index++;

		// if (follow_x) {
		// 	real_t w = p_weights->write[p_index];
		// 	Vector3 v = Vector3(w, 0.0, 0.0);
		// 	p_headings->write[p_index] = goal_transform.xform(v) - origin;
		// 	p_headings->write[p_index+1] = goal_transform.xform(-v) - origin;
		// 	p_index += 2;
		// }

		// if (follow_y) {
		// 	real_t w = p_weights->write[p_index];
		// 	Vector3 v = Vector3(0.0, w, 0.0);
		// 	p_headings->write[p_index] = goal_transform.xform(v) - origin;
		// 	p_headings->write[p_index+1] = goal_transform.xform(-v) - origin;
		// 	p_index += 2;
		// }

		// if (follow_z) {
		// 	real_t w = p_weights->write[p_index];
		// 	Vector3 v = Vector3(0.0, 0.0, w);
		// 	p_headings->write[p_index] = goal_transform.xform(v) - origin;
		// 	p_headings->write[p_index+1] = goal_transform.xform(-v) - origin;
		// 	p_index += 2;
		// }

		/**
		 * The following block makes the headings to be centered at the origin
		 */
		// if (follow_x) {
		// 	real_t w = p_weights->write[p_index];
		// 	Vector3 v = goal_transform.xform(Vector3(w, 0.0, 0.0)) - goal_transform.origin;
		// 	p_headings->write[p_index] = v;
		// 	p_headings->write[p_index+1] = -v;
		// 	p_index += 2;
		// }

		// if (follow_y) {
		// 	real_t w = p_weights->write[p_index];
		// 	Vector3 v = goal_transform.xform(Vector3(0.0, w, 0.0)) - goal_transform.origin;
		// 	p_headings->write[p_index] = v;
		// 	p_headings->write[p_index+1] = -v;
		// 	p_index += 2;
		// }

		// if (follow_z) {
		// 	real_t w = p_weights->write[p_index];
		// 	Vector3 v = goal_transform.xform(Vector3(0.0, 0.0, w)) - goal_transform.origin;
		// 	p_headings->write[p_index] = v;
		// 	p_headings->write[p_index+1] = -v;
		// 	p_index += 2;
		// }

		/**
		 * The following block is a simplified version
		 */
		real_t w = p_weights->write[p_index];
		Vector3 v = goal_transform.xform(Vector3(w, w, w)) - goal_transform.origin;
		p_headings->write[p_index] = v;
		p_headings->write[p_index + 1] = -v;
		p_index += 2;
	} else {
		Vector3 v = goal_transform.origin - origin;
		p_headings->write[p_index] = v;
		p_headings->write[p_index + 1] = -v;
		p_index += 2;
	}
}

void IKEffector3D::update_tip_headings(Ref<IKBone3D> p_for_bone, Vector<Vector3> *p_headings, int32_t &p_index) const {
	Vector3 origin = p_for_bone->get_global_transform().origin;
	Transform tip_xform = for_bone->get_global_transform();
	if (p_for_bone == for_bone) {
		/**
		 * The following block corresponds to the original implementation
		*/
		// real_t scale_by = 1.0; //MAX(origin.distance_to(goal_transform.origin), MIN_SCALE);
		// p_headings->write[p_index] = tip_xform.origin - origin;
		// p_index++;

		// if (follow_x) {
		// 	Vector3 v = Vector3(scale_by, 0.0, 0.0);
		// 	p_headings->write[p_index] = tip_xform.xform(v) - origin;
		// 	p_headings->write[p_index+1] = tip_xform.xform(-v) - origin;
		// 	p_index += 2;
		// }

		// if (follow_y) {
		// 	Vector3 v = Vector3(0.0, scale_by, 0.0);
		// 	p_headings->write[p_index] = tip_xform.xform(v) - origin;
		// 	p_headings->write[p_index+1] = tip_xform.xform(-v) - origin;
		// 	p_index += 2;
		// }

		// if (follow_z) {
		// 	Vector3 v = Vector3(0.0, 0.0, scale_by);
		// 	p_headings->write[p_index] = tip_xform.xform(v) - origin;
		// 	p_headings->write[p_index+1] = tip_xform.xform(-v) - origin;
		// 	p_index += 2;
		// }

		/**
		 * The following block makes the headings to be centered at the origin
		 */
		// real_t scale_by = MAX(origin.distance_to(goal_transform.origin), MIN_SCALE);
		// if (follow_x) {
		// 	Vector3 v = tip_xform.xform(Vector3(scale_by, 0.0, 0.0)) - origin;
		// 	p_headings->write[p_index] = v;
		// 	p_headings->write[p_index+1] = -v;
		// 	p_index += 2;
		// }

		// if (follow_y) {
		// 	Vector3 v = tip_xform.xform(Vector3(0.0, scale_by, 0.0)) - origin;
		// 	p_headings->write[p_index] = v;
		// 	p_headings->write[p_index+1] = -v;
		// 	p_index += 2;
		// }

		// if (follow_z) {
		// 	Vector3 v = tip_xform.xform(Vector3(0.0, 0.0, scale_by)) - origin;
		// 	p_headings->write[p_index] = v;
		// 	p_headings->write[p_index+1] = -v;
		// 	p_index += 2;
		// }

		/**
		 * The following block is a simplified version
		 */
		Vector3 v = tip_xform.xform(Vector3(0.0, 1.0, 0.0)) - tip_xform.origin;
		p_headings->write[p_index] = v;
		p_headings->write[p_index + 1] = -v;
	} else {
		Vector3 v = tip_xform.origin - origin;
		p_headings->write[p_index] = v;
		p_headings->write[p_index + 1] = -v;
		p_index += 2;
	}
}

void IKEffector3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_target_transform", "transform"),
			&IKEffector3D::set_target_transform);
	ClassDB::bind_method(D_METHOD("get_target_transform"),
			&IKEffector3D::get_target_transform);

	ClassDB::bind_method(D_METHOD("set_target_node", "node"),
			&IKEffector3D::set_target_node);
	ClassDB::bind_method(D_METHOD("get_target_node"),
			&IKEffector3D::get_target_node);
}

IKEffector3D::IKEffector3D(const Ref<IKBone3D> &p_for_bone) {
	for_bone = p_for_bone;
	update_priorities();
}
