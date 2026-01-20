#pragma once

struct Parameter
{
	Parameter();

	// 기본
	int32_t m_imageBitCount;
	int32_t m_imageSizeX;
	int32_t m_imageSizeY;
	int32_t m_saveImageCount;
	double m_px2mm;
	double m_FWHM_IncStep_mm;
	int32_t m_FWHM_IncStepCount;
	
	// 프로젝션 관련
	double m_illuminationIncidence_theta_degree;		// 구면좌표계
	double m_illuminationIncidence_phi_degree;			// 구면좌표계
	double m_illuminationIntensity_gv;
	double m_illuminationGratingPitch_mm;
	double m_illuminationGratingPhaseOffset_deg;

	// 배경 형상 : z = axx + bxy + cyy + dx + dy
	double m_backgroundShape_xx;
	double m_backgroundShape_xy;
	double m_backgroundShape_yy;
	double m_backgroundShape_x;
	double m_backgroundShape_y;
	double m_backgroundShape_z0;

	// 물체 형상
	double m_gaussainShapePositionX_mm;
	double m_gaussainShapePositionY_mm;
	double m_gaussianShapeDepth_mm;
	double m_gaussianShapeFWHM_mm;

	// 표면 반사 관련
	double m_surfaceNormalRandomLimitX_degree;
	double m_surfaceNormalRandomLimitY_degree;
	double m_reflectionDistribution_rad;				// 전반사각 기준으로 한 반사광의 퍼짐으로, 반짝이는 성질이 클수록 작은 값을 사용한다.

	// Point Spread Function
	double m_pointSpreadFunctionSigma;
	int32_t m_pointSpreadFunctionKernelSize;

	// 이미지 관련
	double m_modulationIntensityOffset_gv;
	double m_poissonNoiseLimit_ratio;
	double m_gaussianNoiseSigma_gv;

	// FFT 관련
	BOOL m_fft_1D;
	BOOL m_fft_boundaryMirroring;
	BOOL m_fft_useHanningWindow;
	int32_t m_fft_carrierFrequencyX;
	int32_t m_fft_frequencyBandWidthX;
	int32_t m_fft_frequencyBandWidthY;
	int32_t m_fft_butterworthFilterOrder;

	// Debugging
	BOOL m_loadVerticalFringeImage;
	int32_t m_fft_fringeReconstructionGain;
};

