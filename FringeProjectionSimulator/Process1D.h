#pragma once

class ImageLogger;
struct Parameter;

class Process1D
{
public:
	static IPVM::Status Process(const Parameter& parameter, const IPVM::Image_16u_C1& src, ImageLogger& imageLogger);

private:
	static IPVM::Status MakeFFTSource(const IPVM::Image_16u_C1& src, IPVM::Image_32f_C2& fftBuffer);
	static IPVM::Status HanningWindow(IPVM::Image_32f_C2& fftBuffer);
	static IPVM::Status InverseFFTPostProcess(IPVM::Image_32f_C2& fftBuffer);
	static IPVM::Status HanningWindowInv(const Parameter& parameter, IPVM::Image_32f_C2& fftBuffer);

	static IPVM::Status LogFrequencyMagnitude(const IPVM::Image_32f_C2& fftBuffer, ImageLogger& imageLogger);
};

