#include "pch.h"
#include "Process1D.h"

#include "ImageLogger.h"
#include "Parameter.h"
#include "Process.h"

#include "Algorithm/FastFourierTransform1D.h"
#include "Algorithm/ImageProcessing.h"
#include "Algorithm/SignalProcessing.h"
#include "Base/ColorMapIndex.h"
#include "Base/Status.h"
#include "Types/Image_16u_C1.h"
#include "Types/Image_32f_C1.h"
#include "Types/Image_32f_C2.h"
#include "Types/Point_32f_C2.h"

#include <cmath>
#include <ppl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IPVM::Status Process1D::Process(const Parameter& parameter, const IPVM::Image_16u_C1& src, ImageLogger& imageLogger)
{
	const auto imageSizeX = src.GetSizeX();
	const auto imageSizeY = src.GetSizeY();

	IPVM::Image_16u_C1 tempImage(imageSizeX, imageSizeY);

	IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::Copy(src, IPVM::Rect(src), tempImage));

	if (parameter.m_fft_boundaryMirroring)
	{
		IPVM_RUNTIME_CHECK(Process::MirrorBoundary(parameter, tempImage));

		imageLogger.Push(_T("Mirrored Image"), tempImage);
	}

	IPVM::Image_32f_C2 fftBuffer(imageSizeX, imageSizeY);

	IPVM_RUNTIME_CHECK(MakeFFTSource(tempImage, fftBuffer));

	if (parameter.m_fft_useHanningWindow)
	{
		IPVM_RUNTIME_CHECK(HanningWindow(fftBuffer));
	}

	IPVM::FastFourierTransform1D fft;

	for (int32_t y = 0; y < imageSizeY; y++)
	{
		IPVM_RUNTIME_CHECK(fft.Forward(imageSizeX, fftBuffer.GetMem(0, y)));
	}

	IPVM_RUNTIME_CHECK(LogFrequencyMagnitude(fftBuffer, imageLogger));

	IPVM::Image_32f_C1 fftFilterMask(imageSizeX, imageSizeY);

	IPVM_RUNTIME_CHECK(Process::MakeFilterMask1(parameter, fftFilterMask));

	imageLogger.Push(_T("FFT Filter Mask"), fftFilterMask, IPVM::ColorMapIndex::Black_White);

	IPVM_RUNTIME_CHECK(Process::Masking(fftFilterMask, fftBuffer));

	IPVM_RUNTIME_CHECK(LogFrequencyMagnitude(fftBuffer, imageLogger));

	for (int32_t y = 0; y < imageSizeY; y++)
	{
		IPVM_RUNTIME_CHECK(fft.Inverse(imageSizeX, fftBuffer.GetMem(0, y)));
	}

	IPVM_RUNTIME_CHECK(InverseFFTPostProcess(fftBuffer));

	if (parameter.m_fft_useHanningWindow)
	{
		IPVM_RUNTIME_CHECK(HanningWindowInv(parameter, fftBuffer));
	}

	IPVM::Image_32f_C1 phaseMap(imageSizeX, imageSizeY);

	IPVM_RUNTIME_CHECK(Process::MakePhase(parameter, fftBuffer, phaseMap, imageLogger));

	IPVM::Image_32f_C1 zMap(imageSizeX, imageSizeY);

	IPVM_RUNTIME_CHECK(Process::MakeZmap(parameter, phaseMap, zMap, imageLogger));

	IPVM_RUNTIME_CHECK(Process::RemoveBackgroundShape(parameter, zMap, imageLogger));

	return IPVM::Status::success;
}

IPVM::Status Process1D::MakeFFTSource(const IPVM::Image_16u_C1& src, IPVM::Image_32f_C2& fftBuffer)
{
	const auto imageSizeX = fftBuffer.GetSizeX();
	const auto imageSizeY = fftBuffer.GetSizeY();

	fftBuffer.FillZero();

	concurrency::parallel_for(int32_t(0), imageSizeY,
		[&](int32_t y)
		{
			const auto* mem_src = src.GetMem(0, y);
			auto* mem_fftBuffer = fftBuffer.GetMem(0, y);

			double mean = 0.;
			IPVM::ImageProcessing::GetMean(src, IPVM::Rect(0, y, imageSizeX, y + 1), mean);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				if (x & 0x01)
				{
					mem_fftBuffer[x].m_x = static_cast<float>(-mem_src[x] - mean);
				}
				else
				{
					mem_fftBuffer[x].m_x = static_cast<float>(mem_src[x] + mean);
				}
			}
		}
	);

	return IPVM::Status::success;
}

IPVM::Status Process1D::HanningWindow(IPVM::Image_32f_C2& fftBuffer)
{
	const auto imageSizeX = fftBuffer.GetSizeX();
	const auto imageSizeY = fftBuffer.GetSizeY();

	std::vector<float> horHanning(imageSizeX);
	IPVM_RUNTIME_CHECK(IPVM::SignalProcessing::MakeHanningWindow(imageSizeX, &horHanning[0]));

	concurrency::parallel_for(int32_t(0), imageSizeY,
		[&](int32_t y)
		{
			auto* mem_fftBuffer = fftBuffer.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				mem_fftBuffer[x] *= horHanning[x];
			}
		}
	);

	return IPVM::Status::success;
}

IPVM::Status Process1D::InverseFFTPostProcess(IPVM::Image_32f_C2& fftBuffer)
{
	const auto imageSizeX = fftBuffer.GetSizeX();
	const auto imageSizeY = fftBuffer.GetSizeY();

	concurrency::parallel_for(int32_t(0), imageSizeY,
		[&](int32_t y)
		{
			auto* mem_fftBuffer = fftBuffer.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				if (x & 0x01)
				{
					mem_fftBuffer[x] *= -1;
				}

				mem_fftBuffer[x].m_x = static_cast<float>(mem_fftBuffer[x].m_x / imageSizeX);
				mem_fftBuffer[x].m_y = static_cast<float>(mem_fftBuffer[x].m_y / imageSizeX);
			}
		}
	);

	return IPVM::Status::success;
}

IPVM::Status Process1D::HanningWindowInv(const Parameter& parameter, IPVM::Image_32f_C2& fftBuffer)
{
	const auto refSizeX = parameter.m_imageSizeX;
	const auto refSizeY = parameter.m_imageSizeY;

	const auto imageSizeX = fftBuffer.GetSizeX();
	const auto imageSizeY = fftBuffer.GetSizeY();

	const auto offsetX = (imageSizeX - refSizeX) / 2;
	const auto offsetY = (imageSizeY - refSizeY) / 2;

	std::vector<float> horHanning(imageSizeX);
	IPVM_RUNTIME_CHECK(IPVM::SignalProcessing::MakeHanningWindow(imageSizeX, &horHanning[0]));

	concurrency::parallel_for(int32_t(offsetY), imageSizeY - offsetY,
		[&](int32_t y)
		{
			auto* mem_fftBuffer = fftBuffer.GetMem(0, y);

			for (int32_t x = offsetX; x < imageSizeX - offsetX; x++)
			{
				if (horHanning[x] > 0.f)
				{
					mem_fftBuffer[x] /= horHanning[x];
				}
			}
		}
	);

	return IPVM::Status::success;
}

IPVM::Status Process1D::LogFrequencyMagnitude(const IPVM::Image_32f_C2& fftBuffer, ImageLogger& imageLogger)
{
	const auto imageSizeX = fftBuffer.GetSizeX();
	const auto imageSizeY = fftBuffer.GetSizeY();

	IPVM::Image_32f_C1 fftAmplitudeLn(imageSizeX, imageSizeY);

	concurrency::parallel_for(int32_t(0), imageSizeY,
		[&](int32_t y)
		{
			const auto* mem_fftBuffer = fftBuffer.GetMem(0, y);
			auto* mem_fftAmplitudeLn = fftAmplitudeLn.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				mem_fftAmplitudeLn[x] = std::log(1 + std::sqrt(mem_fftBuffer[x].m_x * mem_fftBuffer[x].m_x + mem_fftBuffer[x].m_y * mem_fftBuffer[x].m_y));
			}
		}
	);

	const int32_t halfArea = 10;
	const IPVM::Rect dcArea(imageSizeX / 2 - halfArea, 0, imageSizeX / 2 + halfArea + 1, imageSizeY);

	IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::Fill(dcArea, 0.f, fftAmplitudeLn));

	imageLogger.Push(_T("FFT Amplitude Map"), fftAmplitudeLn, IPVM::ColorMapIndex::NipySpectral);

	return IPVM::Status::success;
}
