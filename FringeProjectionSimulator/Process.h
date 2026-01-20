#pragma once

class ImageLogger;
struct Parameter;

class Process
{
public:
	static IPVM::Rect GetValidROI(const Parameter& parameter, const IPVM::Image& image);

	static IPVM::Status MirrorBoundary(const Parameter& parameter, IPVM::Image_16u_C1& dst);
	static IPVM::Status MakeFilterMask1(const Parameter& parameter, IPVM::Image_32f_C1& mask);
	static IPVM::Status Masking(const IPVM::Image_32f_C1& mask, IPVM::Image_32f_C2& fftBuffer);
	static IPVM::Status MakePhase(const Parameter& parameter, const IPVM::Image_32f_C2& fftBuffer, IPVM::Image_32f_C1& phaseMap, ImageLogger& imageLogger);
	static IPVM::Status MakeZmap(const Parameter& parameter, const IPVM::Image_32f_C1& phaseMap, IPVM::Image_32f_C1& zMap, ImageLogger& imageLogger);
	static IPVM::Status RemoveBackgroundShape(const Parameter& parameter, const IPVM::Image_32f_C1& zMap, ImageLogger& imageLogger);
};

