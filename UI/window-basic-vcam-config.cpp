#include "moc_window-basic-vcam-config.cpp"
#include "window-basic-main.hpp"

#include <qt-wrappers.hpp>
#include <util/util.hpp>
#include <util/platform.h>

#include <QStandardItem>

OBSBasicVCamConfig::OBSBasicVCamConfig(const VCamConfig &_config, bool _vcamActive, QWidget *parent)
	: config(_config),
	  vcamActive(_vcamActive),
	  activeType(_config.type),
	  QDialog(parent),
	  ui(new Ui::OBSBasicVCamConfig)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	ui->outputType->addItem(QTStr("Basic.VCam.OutputType.Program"), (int)VCamOutputType::ProgramView);
	ui->outputType->addItem(QTStr("StudioMode.Preview"), (int)VCamOutputType::PreviewOutput);
	ui->outputType->addItem(QTStr("Basic.Scene"), (int)VCamOutputType::SceneOutput);
	ui->outputType->addItem(QTStr("Basic.Main.Source"), (int)VCamOutputType::SourceOutput);

	ui->outputType->setCurrentIndex(ui->outputType->findData((int)config.type));
	OutputTypeChanged();
	connect(ui->outputType, &QComboBox::currentIndexChanged, this, &OBSBasicVCamConfig::OutputTypeChanged);

	if (VIRTUAL_CAM_ID) {
		ui->camera1Output->addItem(QT_UTF8(obs_output_get_display_name(VIRTUAL_CAM_ID)), VIRTUAL_CAM_ID);
		ui->camera2Output->addItem(QT_UTF8(obs_output_get_display_name(VIRTUAL_CAM_ID)), VIRTUAL_CAM_ID);
	}
	if (VIRTUAL_CAM_2_ID) {
		ui->camera1Output->addItem(QT_UTF8(obs_output_get_display_name(VIRTUAL_CAM_2_ID)), VIRTUAL_CAM_2_ID);
		ui->camera2Output->addItem(QT_UTF8(obs_output_get_display_name(VIRTUAL_CAM_2_ID)), VIRTUAL_CAM_2_ID);
	}
	ui->camera2Output->addItem(QTStr("Basic.VCam.Cameras.Output.Disabled"), "");
	auto camera1OutputIndex = ui->camera1Output->findData(config.camera1Output);
	if (camera1OutputIndex < 0)
		camera1OutputIndex = 0;
	ui->camera1Output->setCurrentIndex(camera1OutputIndex);
	auto camera2OutputIndex = ui->camera2Output->findData(config.camera2Output);
	if (camera2OutputIndex < 0)
		camera2OutputIndex = ui->camera2Output->count - 1;
	ui->camera2Output->setCurrentIndex(camera2OutputIndex);
	ui->camera2Output->setVisible(ui->camera2Output->count > 1);
	ui->camera1Output->setVisible(!ui->camera2Output->visible && ui->camera1Output->count > 1);

	CamerasOutputsChanged();
	connect(ui->camera1Output, &QComboBox::currentIndexChanged, this, &OBSBasicVCamConfig::CamerasOutputsChanged);
	connect(ui->camera2Output, &QComboBox::currentIndexChanged, this, &OBSBasicVCamConfig::CamerasOutputsChanged);

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &OBSBasicVCamConfig::UpdateConfig);
}

void OBSBasicVCamConfig::OutputTypeChanged()
{
	VCamOutputType type = (VCamOutputType)ui->outputType->currentData().toInt();
	ui->outputSelection->setDisabled(false);

	auto list = ui->outputSelection;
	list->clear();

	switch (type) {
	case VCamOutputType::Invalid:
	case VCamOutputType::ProgramView:
	case VCamOutputType::PreviewOutput:
		ui->outputSelection->setDisabled(true);
		list->addItem(QTStr("Basic.VCam.OutputSelection.NoSelection"));
		break;
	case VCamOutputType::SceneOutput: {
		// Scenes in default order
		BPtr<char *> scenes = obs_frontend_get_scene_names();
		for (char **temp = scenes; *temp; temp++) {
			list->addItem(*temp);

			if (config.scene.compare(*temp) == 0)
				list->setCurrentIndex(list->count() - 1);
		}
		break;
	}
	case VCamOutputType::SourceOutput: {
		// Sources in alphabetical order
		std::vector<std::string> sources;
		auto AddSource = [&](obs_source_t *source) {
			auto name = obs_source_get_name(source);

			if (!(obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO))
				return;

			sources.push_back(name);
		};
		using AddSource_t = decltype(AddSource);

		obs_enum_sources(
			[](void *data, obs_source_t *source) {
				auto &AddSource = *static_cast<AddSource_t *>(data);
				if (!obs_source_removed(source))
					AddSource(source);
				return true;
			},
			static_cast<void *>(&AddSource));

		// Sort and select current item
		sort(sources.begin(), sources.end());
		for (auto &&source : sources) {
			list->addItem(source.c_str());

			if (config.source == source)
				list->setCurrentIndex(list->count() - 1);
		}
		break;
	}
	}

	if (!vcamActive)
		return;

	requireRestart = (activeType == VCamOutputType::ProgramView && type != VCamOutputType::ProgramView) ||
			 (activeType != VCamOutputType::ProgramView && type == VCamOutputType::ProgramView);

	ui->warningLabel->setVisible(requireRestart);
}

void OBSBasicVCamConfig::CamerasOutputsChanged()
{
	auto *cameraChangedOutputList = qobject_cast<QComboBox *>(sender());
	auto *camera1OutputList = ui->camera1Output;
	auto *camera2OutputList = ui->camera2Output;

	auto camera1OutputIndex = camera1OutputList->currentIndex();
	auto camera2OutputIndex = camera2OutputList->currentIndex();

	if (camera1OutputIndex == camera2OutputIndex) {
		camera2OutputList->setCurrentIndex(camera2OutputList->count - 1);
		return;
	} else if (cameraChangedOutputList->currentIndex() == -1) {
		if (cameraChangedOutputList == camera1OutputList)
			cameraChangedOutputList->setCurrentIndex(1);
		else
			cameraChangedOutputList->setCurrentIndex(cameraChangedOutputList->count - 1);
		return;
	}
}

void OBSBasicVCamConfig::UpdateConfig()
{
	VCamOutputType type = (VCamOutputType)ui->outputType->currentData().toInt();
	switch (type) {
	case VCamOutputType::ProgramView:
	case VCamOutputType::PreviewOutput:
		break;
	case VCamOutputType::SceneOutput:
		config.scene = ui->outputSelection->currentText().toStdString();
		break;
	case VCamOutputType::SourceOutput:
		config.source = ui->outputSelection->currentText().toStdString();
		break;
	default:
		// unknown value, don't save type
		return;
	}

	config.type = type;

	config.camera1Output = camera1OutputList->currentData().toStdString();
	config.camera2Output = camera2OutputList->currentData().toStdString();

	if (requireRestart) {
		emit AcceptedAndRestart(config);
	} else {
		emit Accepted(config);
	}
}
