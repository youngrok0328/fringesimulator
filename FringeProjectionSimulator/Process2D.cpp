#include "pch.h"
#include "Process2D.h"

#include "ImageLogger.h"
#include "Parameter.h"
#include "Process.h"

#include "Algorithm/FastFourierTransform2D.h"
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

IPVM::Status Process2D::Process(const Parameter& parameter, const IPVM::Image_16u_C1& src, ImageLogger& imageLogger)
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

	IPVM::FastFourierTransform2D fft;

	IPVM_RUNTIME_CHECK(fft.Forward(fftBuffer));

	IPVM_RUNTIME_CHECK(LogFrequencyMagnitude(fftBuffer, imageLogger));

	IPVM::Image_32f_C1 fftFilterMask(imageSizeX, imageSizeY);

	IPVM_RUNTIME_CHECK(MakeFilterMask2(parameter, fftFilterMask));

	imageLogger.Push(_T("FFT Filter Mask 2D"), fftFilterMask, IPVM::ColorMapIndex::Black_White);

	IPVM_RUNTIME_CHECK(Process::Masking(fftFilterMask, fftBuffer));

	IPVM_RUNTIME_CHECK(LogFrequencyMagnitude(fftBuffer, imageLogger));

	IPVM_RUNTIME_CHECK(fft.Inverse(fftBuffer));

	IPVM_RUNTIME_CHECK(InverseFFTPostProcess(fftBuffer));

	if (parameter.m_fft_useHanningWindow)
	{
		IPVM_RUNTIME_CHECK(HanningWindowInv(fftBuffer));
	}

	IPVM::Image_32f_C1 phaseMap(imageSizeX, imageSizeY);

	IPVM_RUNTIME_CHECK(Process::MakePhase(parameter, fftBuffer, phaseMap, imageLogger));

	IPVM::Image_32f_C1 zMap(imageSizeX, imageSizeY);

	IPVM_RUNTIME_CHECK(Process::MakeZmap(parameter, phaseMap, zMap, imageLogger));

	IPVM_RUNTIME_CHECK(Process::RemoveBackgroundShape(parameter, zMap, imageLogger));

	return IPVM::Status::success;
}

IPVM::Status Process2D::MakeFFTSource(const IPVM::Image_16u_C1& src, IPVM::Image_32f_C2& fftBuffer)
{
	const auto imageSizeX = fftBuffer.GetSizeX();
	const auto imageSizeY = fftBuffer.GetSizeY();

	fftBuffer.FillZero();

	double mean = 0.;
	IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::GetMean(src, IPVM::Rect(src), mean));

	concurrency::parallel_for(int32_t(0), imageSizeY,
		[&](int32_t y)
		{
			const auto* mem_src = src.GetMem(0, y);
			auto* mem_fftBuffer = fftBuffer.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				if ((x + y) & 0x01)
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

IPVM::Status Process2D::HanningWindow(IPVM::Image_32f_C2& fftBuffer)
{
	const auto imageSizeX = fftBuffer.GetSizeX();
	const auto imageSizeY = fftBuffer.GetSizeY();

	std::vector<float> horHanning(imageSizeX);
	IPVM_RUNTIME_CHECK(IPVM::SignalProcessing::MakeHanningWindow(imageSizeX, &horHanning[0]));

	std::vector<float> verHanning(imageSizeY);
	IPVM_RUNTIME_CHECK(IPVM::SignalProcessing::MakeHanningWindow(imageSizeY, &verHanning[0]));

	concurrency::parallel_for(int32_t(0), imageSizeY,
		[&](int32_t y)
		{
			auto* mem_fftBuffer = fftBuffer.GetMem(0, y);

			const auto verHanningValue = verHanning[y];

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				mem_fftBuffer[x] *= verHanningValue * horHanning[x];
			}
		}
	);

	return IPVM::Status::success;
}

IPVM::Status Process2D::MakeFilterMask2(const Parameter& parameter, IPVM::Image_32f_C1& mask)
{
	IPVM_RUNTIME_CHECK(Process::MakeFilterMask1(parameter, mask));

	const auto fftSizeX = mask.GetSizeX();
	const auto fftSizeY = mask.GetSizeY();

	const int32_t dcIndex_x = fftSizeX / 2;
	const int32_t dcIndex_y = fftSizeY / 2;

	const int32_t halfBandWidth = parameter.m_fft_frequencyBandWidthY / 2;

	const int32_t beginy = max(0, dcIndex_y - parameter.m_fft_frequencyBandWidthY);
	const int32_t endy = min(fftSizeY, dcIndex_y + parameter.m_fft_frequencyBandWidthY + 1);

	for (int32_t y = 0; y < beginy; y++)
	{
		::memset(mask.GetMem(dcIndex_x, y), 0, sizeof(float) * dcIndex_x);
	}

	concurrency::parallel_for(int32_t(beginy), endy,
		[&](int32_t y)
		{
			auto* mem_mask = mask.GetMem(0, y);

			const double dr = std::fabs(double(y - dcIndex_y));
			const double dr_per_cutoff = dr / halfBandWidth;

			const double power_2_n = ::pow(dr_per_cutoff, 2. * parameter.m_fft_butterworthFilterOrder);

			const float coef = float(1 / (1 + power_2_n));

			for (int32_t x = dcIndex_x; x < fftSizeX; x++)
			{
				mem_mask[x] *= coef;
			}
		}
	);

	for (int32_t y = endy; y < fftSizeY; y++)
	{
		::memset(mask.GetMem(dcIndex_x, y), 0, sizeof(float) * dcIndex_x);
	}

	return IPVM::Status::success;
}

IPVM::Status Process2D::InverseFFTPostProcess(IPVM::Image_32f_C2& fftBuffer)
{
	const auto imageSizeX = fftBuffer.GetSizeX();
	const auto imageSizeY = fftBuffer.GetSizeY();

	const double NN = imageSizeX * imageSizeY;

	concurrency::parallel_for(int32_t(0), imageSizeY,
		[&](int32_t y)
		{
			auto* mem_fftBuffer = fftBuffer.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				if ((x + y) & 0x01)
				{
					mem_fftBuffer[x] *= -1;
				}

				mem_fftBuffer[x].m_x = static_cast<float>(mem_fftBuffer[x].m_x / NN);
				mem_fftBuffer[x].m_y = static_cast<float>(mem_fftBuffer[x].m_y / NN);
			}
		}
	);

	return IPVM::Status::success;
}

IPVM::Status Process2D::HanningWindowInv(IPVM::Image_32f_C2& fftBuffer)
{
	const auto imageSizeX = fftBuffer.GetSizeX();
	const auto imageSizeY = fftBuffer.GetSizeY();

	std::vector<float> horHanning(imageSizeX);
	IPVM_RUNTIME_CHECK(IPVM::SignalProcessing::MakeHanningWindow(imageSizeX, &horHanning[0]));

	std::vector<float> verHanning(imageSizeY);
	IPVM_RUNTIME_CHECK(IPVM::SignalProcessing::MakeHanningWindow(imageSizeY, &verHanning[0]));

	concurrency::parallel_for(int32_t(0), imageSizeY,
		[&](int32_t y)
		{
			auto* mem_fftBuffer = fftBuffer.GetMem(0, y);

			const auto verHanningValue = verHanning[y];

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				const auto value = verHanningValue * horHanning[x];

				if (value > 0.f)
				{
					mem_fftBuffer[x] /= value;
				}
			}
		}
	);

	return IPVM::Status::success;
}

IPVM::Status Process2D::LogFrequencyMagnitude(const IPVM::Image_32f_C2& fftBuffer, ImageLogger& imageLogger)
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
	const IPVM::Rect dcArea(imageSizeX / 2 - halfArea, imageSizeY / 2 - halfArea, imageSizeX / 2 + halfArea + 1, imageSizeY / 2 + halfArea + 1);

	IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::Fill(dcArea, 0.f, fftAmplitudeLn));

	imageLogger.Push(_T("FFT Amplitude Map"), fftAmplitudeLn, IPVM::ColorMapIndex::NipySpectral);

	return IPVM::Status::success;
}
