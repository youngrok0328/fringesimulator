#pragma once

class ImageLogger;
struct Parameter;

class Process2D
{
public:
	static IPVM::Status Process(const Parameter& parameter, const IPVM::Image_16u_C1& src, ImageLogger& imageLogger);

private:
	static IPVM::Status MakeFFTSource(const IPVM::Image_16u_C1& src, IPVM::Image_32f_C2& fftBuffer);
	static IPVM::Status HanningWindow(IPVM::Image_32f_C2& fftBuffer);
	static IPVM::Status MakeFilterMask2(const Parameter& parameter, IPVM::Image_32f_C1& mask);
	static IPVM::Status InverseFFTPostProcess(IPVM::Image_32f_C2& fftBuffer);
	static IPVM::Status HanningWindowInv(IPVM::Image_32f_C2& fftBuffer);

	static IPVM::Status LogFrequencyMagnitude(const IPVM::Image_32f_C2& fftBuffer, ImageLogger& imageLogger);

	friend class Process1D;
};

