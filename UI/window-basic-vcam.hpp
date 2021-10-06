#pragma once

#include <string>

constexpr const char *VIRTUAL_CAM_ID = "virtualcam_output";
constexpr const char *VIRTUAL_CAM_2_ID = nullptr;

enum VCamOutputType {
	Invalid,
	SceneOutput,
	SourceOutput,
	ProgramView,
	PreviewOutput,
};

// Kept for config upgrade
enum VCamInternalType {
	Default,
	Preview,
};

struct VCamConfig {
	VCamOutputType type = VCamOutputType::ProgramView;
	std::string scene;
	std::string source;
	std::string camera1Output;
	std::string camera2Output;
};
