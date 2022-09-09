/*************************************************************************/
/*  ik_bone_3d.h                                                         */
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

#ifndef EWBIK_SHADOW_BONE_3D_H
#define EWBIK_SHADOW_BONE_3D_H

#include "ik_effector_3d.h"
#include "ik_effector_template.h"
#include "kusudama.h"
#include "math/ik_transform.h"

#include "core/io/resource.h"
#include "core/object/ref_counted.h"
#include "scene/3d/skeleton_3d.h"

#define IK_DEFAULT_DAMPENING 0.20944f

class IKEffector3D;
class IKKusudama;

class IKBone3D : public Resource {
	GDCLASS(IKBone3D, Resource);

	BoneId bone_id = -1;
	Ref<IKBone3D> parent;
	Vector<Ref<IKBone3D>> children;
	Ref<IKEffector3D> pin;
	double last_mean_square_deviation = INFINITY;

	float default_dampening = Math_PI;
	float dampening = get_parent().is_null() ? Math_PI : default_dampening;
	float cos_half_dampen = Math::cos(dampening / 2.0f);
	Ref<IKKusudama> constraint;
	// In the space of the local parent bone transform
	// Origin is the origin of the bone direction transform
	// Can be independent and should be calculated
	// to keep -y to be the opposite of its bone forward orientation
	// To avoid singularity that is ambigous.
	Ref<IKTransform3D> constraint_transform = memnew(IKTransform3D());
	Ref<IKTransform3D> transform = memnew(IKTransform3D()); // bone's actual transform
	Ref<IKTransform3D> bone_direction_transform = memnew(IKTransform3D()); // Physical direction of the bone. Calculate Y is the bone up.
protected:
	static void _bind_methods();

public:
	double get_last_mean_square_deviation() {
		return last_mean_square_deviation;
	}
	void set_last_mean_square_deviation(double p_last_mean_square_deviation) {
		last_mean_square_deviation = p_last_mean_square_deviation;
	}
	Ref<IKTransform3D> get_bone_direction_transform();
	void set_bone_direction_transform(Ref<IKTransform3D> p_bone_direction);
	void set_constraint_transform(Ref<IKTransform3D> p_transform);
	Ref<IKTransform3D> get_constraint_transform();
	void add_constraint(Ref<IKKusudama> p_constraint);
	Ref<IKKusudama> get_constraint() const;
	void update_cosine_dampening();
	void set_bone_id(BoneId p_bone_id, Skeleton3D *p_skeleton = nullptr);
	BoneId get_bone_id() const;
	void set_parent(const Ref<IKBone3D> &p_parent);
	Ref<IKBone3D> get_parent() const;
	void set_pin(const Ref<IKEffector3D> &p_pin);
	Ref<IKEffector3D> get_pin() const;
	void set_pose(const Transform3D &p_transform);
	Transform3D get_pose() const;
	void set_global_pose(const Transform3D &p_transform);
	Transform3D get_global_pose() const;
	void set_initial_pose(Skeleton3D *p_skeleton);
	void set_skeleton_bone_pose(Skeleton3D *p_skeleton, real_t p_strength);
	void create_pin();
	bool is_pinned() const;
	Ref<IKTransform3D> get_ik_transform();
	IKBone3D() {}
	IKBone3D(StringName p_bone, Skeleton3D *p_skeleton, const Ref<IKBone3D> &p_parent, Vector<Ref<IKEffectorTemplate>> &p_pins, float p_default_dampening = IK_DEFAULT_DAMPENING);
	~IKBone3D() {}
	float get_cos_half_dampen() const;
	void set_cos_half_dampen(float p_cos_half_dampen);
};

#endif // EWBIK_SHADOW_BONE_3D_H
