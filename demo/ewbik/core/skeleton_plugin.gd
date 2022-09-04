@tool
extends EditorScript

func _run():
	var root : Node3D = get_editor_interface().get_edited_scene_root()
	var queue : Array
	queue.push_back(root)
	var string_builder : Array
	var skeleton : Skeleton3D
	var ewbik : EWBIK = null
	while not queue.is_empty():
		var front = queue.front()
		var node : Node = front
		if node is Skeleton3D:
			skeleton = node
		if node is EWBIK:
			ewbik = node
		var child_count : int = node.get_child_count()
		for i in child_count:
			queue.push_back(node.get_child(i))
		queue.pop_front()
	if ewbik != null:
		ewbik.queue_free()
	ewbik = EWBIK.new()
	skeleton.add_child(ewbik, true)
	ewbik.owner = skeleton.owner
	ewbik.name = "EWBIK"
	ewbik.skeleton = NodePath("..")
	var godot_to_vrm : Dictionary
	var profile : SkeletonProfileHumanoid = SkeletonProfileHumanoid.new()
	var bone_map : BoneMap = BoneMap.new()
	bone_map.profile = profile
	var bone_vrm_mapping : Dictionary
	ewbik.max_ik_iterations = 10
	ewbik.default_damp = deg_to_rad(1)
	ewbik.live_preview = true
	var pin_i = 0
	var bones = ["Root", "Hips", "LeftHand", "RightHand", "Head"]
	ewbik.pin_count = bones.size()
	for bone_name in bones:
		var bone_index = skeleton.find_bone(bone_name)
		var node_3d : Node3D = Node3D.new()
		node_3d.name = bone_name
		skeleton.get_parent().add_child(node_3d)
		node_3d.owner = skeleton.owner
		ewbik.set_pin_bone_name(pin_i, bone_name)
		var bone_id = skeleton.find_bone(bone_name)
		if bone_id == -1:
			pin_i = pin_i + 1
			continue
		var bone_global_pose : Transform3D = skeleton.get_bone_global_rest(bone_id)
		bone_global_pose = skeleton.transform * bone_global_pose
		node_3d.global_transform = bone_global_pose
		ewbik.set_pin_use_node_rotation(pin_i, true)
		if bone_name in ["Root"]:
			ewbik.set_pin_depth_falloff(pin_i, 0)
		var path_string : String = "../../" + str(bone_name)
		ewbik.set_pin_nodepath(pin_i, NodePath(path_string))
		pin_i = pin_i + 1
	ewbik.constraint_count = profile.bone_size
	for constraint_i in profile.bone_size:
		var bone_name : StringName = profile.get_bone_name(constraint_i)
		if bone_name == null:
			continue
		ewbik.set_constraint_name(constraint_i, bone_name)
		# Female age 9 - 19 https://pubmed.ncbi.nlm.nih.gov/32644411/
		if bone_name in ["LeftLowerArm", "RightLowerArm"]:
			ewbik.set_kusudama_twist_from(constraint_i, deg_to_rad(-90))
			ewbik.set_kusudama_twist_to(constraint_i, deg_to_rad(90))
			ewbik.set_kusudama_limit_cone_count(constraint_i, 2)
			ewbik.set_kusudama_limit_cone_center(constraint_i, 0, Vector3(0, 1, 0))
			ewbik.set_kusudama_limit_cone_radius(constraint_i, 0, deg_to_rad(10))
			ewbik.set_kusudama_limit_cone_center(constraint_i, 1, Vector3(1, 1, 0).normalized())
			ewbik.set_kusudama_limit_cone_radius(constraint_i, 1, deg_to_rad(10))
