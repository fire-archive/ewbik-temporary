/*************************************************************************/
/*  ewbik_bone_effector_3d.h                                             */
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
#ifndef EWBIK_BONE_EFFECTOR_3D_H
#define EWBIK_BONE_EFFECTOR_3D_H

#include "core/object/reference.h"
#include "ewbik_shadow_bone_3d.h"
#include "scene/3d/skeleton_3d.h"

class IKBone;

class EWBIKBoneEffector3D : public Reference {
	GDCLASS(EWBIKBoneEffector3D, Reference);
	friend class IKBone;

private:
	Ref<IKBone> for_bone;
	Transform target_transform;
	NodePath target_nodepath = NodePath();
	bool use_target_node_rotation = false;
	Transform goal_transform;
	int32_t num_headings;
	Vector3 priority = Vector3(1.0, 0.0, 1.0);
	real_t weight = 1.0;
	bool follow_x, follow_y, follow_z;

	void update_priorities();
	void update_goal_transform(Skeleton3D *p_skeleton);

protected:
	static void _bind_methods();

public:
	void set_target_transform(const Transform &p_target_transform);
	Transform get_target_transform() const;
	void set_target_node(const NodePath &p_target_node_path);
	NodePath get_target_node() const;
	void set_use_target_node_rotation(bool p_use);
	bool get_use_target_node_rotation() const;
	bool is_following_translation_only() const;
	void update_target_headings(Skeleton3D *p_skeleton, PackedVector3Array &p_headings, Vector<real_t> &p_weights);
	void update_tip_headings(Skeleton3D *p_skeleton, PackedVector3Array &p_headings, int32_t &p_index);

	EWBIKBoneEffector3D(const Ref<IKBone> &p_for_bone);
	~EWBIKBoneEffector3D() {}
};

#endif // EWBIK_BONE_EFFECTOR_3D_H