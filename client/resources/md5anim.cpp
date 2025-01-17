#include "md5anim.hpp"

#include <fstream>
#include <memory>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/compatibility.hpp>
#include <spdlog/spdlog.h>

#include "graphics/animation.hpp"
#include "md5mesh.hpp"

namespace tec {

/**
* \brief Cleans an input string by removing certain grouping characters.
*
* These characters include ", ', (, and ).
* \param[in] std::string str The string to clean.
* \return The cleaned string
*/
extern std::string CleanString(std::string str);

std::shared_ptr<MD5Anim> MD5Anim::Create(const Path& fname) {
	auto anim = std::make_shared<MD5Anim>();
	anim->SetName(fname.Relative().toString());
	anim->SetFileName(fname);

	if (anim->Parse()) {
		for (std::size_t i = 0; i < anim->frames.size(); ++i) {
			anim->BuildFrameSkeleton(i);
		}
		AnimationMap::Set(anim->GetName(), anim);
		return anim;
	}

	spdlog::get("console_log")->warn("[MD5Anim] Error parsing file {}", fname);
	return nullptr;
}

bool MD5Anim::Parse() {
	auto _log = spdlog::get("console_log");
	if (!this->path || !this->path.FileExists()) {
		_log->error("[MD5Anim] Can't open the file {}. Invalid path or missing file.", path);
		// Can't open the file!
		return false;
	}

	auto f = this->path.OpenStream();
	if (!f->is_open()) {
		_log->error("[MD5Anim] Error opening file {}", path);
		return false;
	}

	std::string line;
	unsigned int num_components = 0;
	while (std::getline(*f, line)) {
		std::stringstream ss(line);
		std::string identifier;

		ss >> identifier;
		if (identifier == "MD5Version") {
			int version;
			ss >> version;

			if (version != 10) {
				return false;
			}
		}
		else if (identifier == "numFrames") {
			unsigned int num_frames;
			ss >> num_frames;
			this->frames.reserve(num_frames);
		}
		else if (identifier == "numJoints") {
			unsigned int njoints;
			ss >> njoints;
			this->joints.reserve(njoints);
		}
		else if (identifier == "frameRate") {
			ss >> this->frame_rate;
		}
		else if (identifier == "numAnimatedComponents") {
			ss >> num_components;
		}
		else if (identifier == "hierarchy") {
			while (std::getline(*f, line)) {
				if (line.find("\"") != std::string::npos) {
					ss.str(CleanString(line));
					Joint bone;
					ss >> bone.name >> bone.parent >> bone.flags >> bone.start_index;
					this->joints.push_back(std::move(bone));
				}
				// Check if the line contained the closing brace. This is done after parsing
				// as the line might have the ending brace on it.
				if (line.find("}") != std::string::npos) {
					break;
				}
			}
		}
		else if (identifier == "bounds") {
			while (std::getline(*f, line)) {
				if ((line.find("(") != std::string::npos) && (line.find(")") != std::string::npos)) {
					ss.str(CleanString(line));
					BoundingBox bbox;
					ss >> bbox.min.x >> bbox.min.y >> bbox.min.z;
					ss >> bbox.max.x >> bbox.max.y >> bbox.max.z;
					this->bounds.push_back(std::move(bbox));
				}
				// Check if the line contained the closing brace. This is done after parsing
				// as the line might have the ending brace on it.
				if (line.find("}") != std::string::npos) {
					break;
				}
			}
		}
		else if (identifier == "baseframe") {
			std::size_t index = 0;
			while (std::getline(*f, line)) {
				if ((line.find("(") != std::string::npos) && (line.find(")") != std::string::npos)) {
					ss.str(CleanString(line));
					auto& joint = this->joints[index];
					// Check if the base frame block is malformed.
					if (index > this->joints.size()) {
						return false;
					}
					ss >> joint.base_position.x >> joint.base_position.y >> joint.base_position.z;
					ss >> joint.base_orientation.x >> joint.base_orientation.y >> joint.base_orientation.z;
				}
				// Check if the line contained the closing brace. This is done after parsing
				// as the line might have the ending brace on it.
				if (line.find("}") != std::string::npos) {
					break;
				}
				++index;
			}
		}
		else if (identifier == "frame") {
			Frame frame;
			ss >> frame.index;
			unsigned int number = 0;
			while (std::getline(*f, line)) {
				ss.clear();
				ss.str(line);
				// Check if the line contained the closing brace. This is done after parsing
				// as the line might have the ending brace on it.
				if (line.find("}") != std::string::npos) {
					break;
				}
				while (!ss.eof()) {
					// Check if frame block is malformed.
					if (number > num_components) {
						return false;
					}
					float temp;
					ss >> temp;
					frame.parameters.push_back(temp);
					++number;
				}
			}
			this->frames.push_back(std::move(frame));
		}
	}

	return true;
}

void MD5Anim::BuildFrameSkeleton(std::size_t frame_index) {
	auto& frame = this->frames[frame_index];

	for (const auto& joint : this->joints) {
		unsigned int j = 0;

		// Start with the base frame position and orientation.
		SkeletonJoint skeleton_joint = {joint.base_position, joint.base_orientation};

		if (joint.flags & 1) { // Pos.x
			skeleton_joint.position.x = frame.parameters[joint.start_index + j++];
		}
		if (joint.flags & 2) { // Pos.y
			skeleton_joint.position.y = frame.parameters[joint.start_index + j++];
		}
		if (joint.flags & 4) { // Pos.x
			skeleton_joint.position.z = frame.parameters[joint.start_index + j++];
		}
		if (joint.flags & 8) { // Orient.x
			skeleton_joint.orientation.x = frame.parameters[joint.start_index + j++];
		}
		if (joint.flags & 16) { // Orient.y
			skeleton_joint.orientation.y = frame.parameters[joint.start_index + j++];
		}
		if (joint.flags & 32) { // Orient.z
			skeleton_joint.orientation.z = frame.parameters[joint.start_index + j++];
		}

		ComputeWNeg(skeleton_joint.orientation);

		if (joint.parent >= 0) {
			const auto& parent_joint = frame.skeleton.skeleton_joints[joint.parent];
			glm::vec3 rotPos = parent_joint.orientation * skeleton_joint.position;

			skeleton_joint.position = parent_joint.position + rotPos;
			skeleton_joint.orientation = parent_joint.orientation * skeleton_joint.orientation;

			skeleton_joint.orientation = glm::normalize(skeleton_joint.orientation);
		}

		frame.skeleton.skeleton_joints.push_back(std::move(skeleton_joint));
	}
}

bool MD5Anim::CheckMesh(std::shared_ptr<MD5Mesh> mesh) {
	if (mesh) {
		// Make sure number of joints matches.
		if (this->joints.size() != mesh->joints.size()) {
			return false;
		}

		for (std::size_t i = 0; i < this->joints.size(); ++i) {
			// Make sure joint names and parents match up.
			if ((this->joints[i].name != mesh->joints[i].name) || (this->joints[i].parent != mesh->joints[i].parent)) {
				return false;
			}
			this->joints[i].bind_orientation = glm::conjugate(mesh->joints[i].orientation);
			this->joints[i].bind_position = -mesh->joints[i].position;
		}
		return true;
	}
	return false;
}

void MD5Anim::InterpolatePose(
		std::vector<AnimationBone>& pose_out, std::size_t frame_index_start, std::size_t frame_index_end, float delta) {
	const auto& skeleton0 = this->frames[frame_index_start].skeleton;
	const auto& skeleton1 = this->frames[frame_index_end].skeleton;

	std::size_t num_joints = this->joints.size();

	pose_out.resize(num_joints);

	for (std::size_t i = 0; i < num_joints; ++i) {
		AnimationBone& finalpose = pose_out[i];

		const SkeletonJoint& joint0 = skeleton0.skeleton_joints[i];
		const SkeletonJoint& joint1 = skeleton1.skeleton_joints[i];

		finalpose.offset = glm::vec4(glm::lerp(joint0.position, joint1.position, delta), 0.0);
		finalpose.orientation = glm::mix(joint0.orientation, joint1.orientation, delta);
		finalpose.orientation *= this->joints[i].bind_orientation;
		finalpose.rest = glm::vec4(this->joints[i].bind_position, 0.0);
	}
}
} // namespace tec
