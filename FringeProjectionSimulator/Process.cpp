#include "pch.h"
#include "Process.h"

#include "ImageLogger.h"
#include "Parameter.h"

#include "Algorithm/DataFitting.h"
#include "Algorithm/FastFourierTransform1D.h"
#include "Algorithm/ImageProcessing.h"
#include "Algorithm/PhaseProcessing.h"
#include "Algorithm/Polynomial.h"
#include "Algorithm/SpatialPhaseUnwrapping.h"
#include "Base/ColorMapIndex.h"
#include "Base/Status.h"
#include "Types/Image_16u_C1.h"
#include "Types/Image_32f_C1.h"
#include "Types/Image_32f_C2.h"
#include "Types/Point_32f_C2.h"
#include "Types/Point_32f_C3.h"

#include <cmath>
#include <ppl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IPVM::Rect Process::GetValidROI(const Parameter& parameter, const IPVM::Image& image)
{
	const auto refSizeX = parameter.m_imageSizeX;
	const auto refSizeY = parameter.m_imageSizeY;

	const auto realSizeX = image.GetSizeX();
	const auto realSizeY = image.GetSizeY();

	const auto offsetX = (realSizeX - refSizeX) / 2;
	const auto offsetY = (realSizeY - refSizeY) / 2;

	const IPVM::Rect innerArea(offsetX, offsetY, offsetX + refSizeX, offsetY + refSizeY);

	return innerArea;
}

IPVM::Status Process::MirrorBoundary(const Parameter& parameter, IPVM::Image_16u_C1& dst)
{
	const auto refSizeX = parameter.m_imageSizeX;
	const auto refSizeY = parameter.m_imageSizeY;

	const auto realSizeX = dst.GetSizeX();
	const auto realSizeY = dst.GetSizeY();

	const auto offsetX = (realSizeX - refSizeX) / 2;
	const auto offsetY = (realSizeY - refSizeY) / 2;

	const IPVM::Rect innerArea(offsetX, offsetY, offsetX + refSizeX, offsetY + refSizeY);

	IPVM::Image_16u_C1 refImage(dst, innerArea);

	if constexpr (1)	// Left-Top
	{
		IPVM::Image_16u_C1 refChild(refImage, IPVM::Rect(0, 0, offsetX, offsetY));
		IPVM::Image_16u_C1 dstChild(dst, IPVM::Rect(0, 0, offsetX, offsetY));

		IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::MirrorByAxisXY(refChild, IPVM::Rect(refChild), dstChild));
	}

	if constexpr (1)	// Top
	{
		IPVM::Image_16u_C1 refChild(refImage, IPVM::Rect(0, 0, refSizeX, offsetY));
		IPVM::Image_16u_C1 dstChild(dst, IPVM::Rect(offsetX, 0, offsetX + refSizeX, offsetY));

		IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::MirrorByAxisX(refChild, IPVM::Rect(refChild), dstChild));
	}

	if constexpr (1)	// Right-Top
	{
		IPVM::Image_16u_C1 refChild(refImage, IPVM::Rect(-offsetX + refSizeX, 0, refSizeX, offsetY));
		IPVM::Image_16u_C1 dstChild(dst, IPVM::Rect(-offsetX + realSizeX, 0, realSizeX, offsetY));

		IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::MirrorByAxisXY(refChild, IPVM::Rect(refChild), dstChild));
	}

	if constexpr (1)	// Left
	{
		IPVM::Image_16u_C1 refChild(refImage, IPVM::Rect(0, 0, offsetX, refSizeY));
		IPVM::Image_16u_C1 dstChild(dst, IPVM::Rect(0, offsetY, offsetX, offsetY + refSizeY));

		IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::MirrorByAxisY(refChild, IPVM::Rect(refChild), dstChild));
	}

	if constexpr (1)	// Right
	{
		IPVM::Image_16u_C1 refChild(refImage, IPVM::Rect(-offsetX + refSizeX, 0, refSizeX, refSizeY));
		IPVM::Image_16u_C1 dstChild(dst, IPVM::Rect(-offsetX + realSizeX, offsetY, realSizeX, offsetY + refSizeY));

		IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::MirrorByAxisY(refChild, IPVM::Rect(refChild), dstChild));
	}

	if constexpr (1)	// Left-Bottom
	{
		IPVM::Image_16u_C1 refChild(refImage, IPVM::Rect(0, -offsetX + refSizeY, offsetX, refSizeY));
		IPVM::Image_16u_C1 dstChild(dst, IPVM::Rect(0, -offsetY + realSizeY, offsetX, realSizeY));

		IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::MirrorByAxisXY(refChild, IPVM::Rect(refChild), dstChild));
	}

	if constexpr (1)	// Bottom
	{
		IPVM::Image_16u_C1 refChild(refImage, IPVM::Rect(0, -offsetX + refSizeY, refSizeX, refSizeY));
		IPVM::Image_16u_C1 dstChild(dst, IPVM::Rect(offsetX, -offsetY + realSizeY, -offsetX + realSizeX, realSizeY));

		IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::MirrorByAxisX(refChild, IPVM::Rect(refChild), dstChild));
	}

	if constexpr (1)	// Right-Bottom
	{
		IPVM::Image_16u_C1 refChild(refImage, IPVM::Rect(-offsetX + refSizeX, -offsetX + refSizeY, refSizeX, refSizeY));
		IPVM::Image_16u_C1 dstChild(dst, IPVM::Rect(-offsetX + realSizeX, -offsetY + realSizeY, realSizeX, realSizeY));

		IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::MirrorByAxisXY(refChild, IPVM::Rect(refChild), dstChild));
	}

	return IPVM::Status::success;
}

IPVM::Status Process::MakeFilterMask1(const Parameter& parameter, IPVM::Image_32f_C1& mask)
{
	const auto fftSizeX = IPVM::FastFourierTransform1D().GetFFTLength(parameter.m_imageSizeX);
	const auto fftSizeY = IPVM::FastFourierTransform1D().GetFFTLength(parameter.m_imageSizeY);

	mask.FillZero();

	const int32_t dcIndex_x = fftSizeX / 2;
	const int32_t carrierIndex = dcIndex_x + parameter.m_fft_carrierFrequencyX;
	const int32_t halfBandWidth = parameter.m_fft_frequencyBandWidthX / 2;

	auto* mem_mask = mask.GetMem(0, 0);

	const int32_t beginx = max(dcIndex_x, carrierIndex - parameter.m_fft_frequencyBandWidthX);
	const int32_t endx = min(fftSizeX, carrierIndex + parameter.m_fft_frequencyBandWidthX + 1);

	for (int32_t x = beginx; x < endx; x++)
	{
		const double dr = ::fabs(double(x - carrierIndex));
		const double dr_per_cutoff = dr / halfBandWidth;

		const double power_2_n = ::pow(dr_per_cutoff, 2. * parameter.m_fft_butterworthFilterOrder);

		mem_mask[x] = float(1 / (1 + power_2_n));
	}

	for (int32_t y = 1; y < fftSizeY; y++)
	{
		::memcpy(mask.GetMem(0, y), mem_mask, fftSizeX * sizeof(float));
	}

	return IPVM::Status::success;
}

IPVM::Status Process::Masking(const IPVM::Image_32f_C1& mask, IPVM::Image_32f_C2& fftBuffer)
{
	const auto imageSizeX = mask.GetSizeX();
	const auto imageSizeY = mask.GetSizeY();

	for (int32_t y = 0; y < imageSizeY; y++)
	{
		const auto* mem_mask = mask.GetMem(0, y);
		auto* mem_fftBuffer = fftBuffer.GetMem(0, y);

		for (int32_t x = 0; x < imageSizeX; x++)
		{
			mem_fftBuffer[x] *= mem_mask[x];
		}
	}

	return IPVM::Status::success;
}

IPVM::Status Process::MakePhase(const Parameter& parameter, const IPVM::Image_32f_C2& fftBuffer, IPVM::Image_32f_C1& phaseMap, ImageLogger& imageLogger)
{
	const auto imageSizeX = fftBuffer.GetSizeX();
	const auto imageSizeY = fftBuffer.GetSizeY();

	const auto validRoi = GetValidROI(parameter, fftBuffer);

	IPVM::Image_16u_C1 outputImageCos(imageSizeX, imageSizeY);
	IPVM::Image_16u_C1 outputImageSin(imageSizeX, imageSizeY);
	IPVM::Image_32f_C1 outputAmplitude(imageSizeX, imageSizeY);
	IPVM::Image_32f_C1 outputPhase(imageSizeX, imageSizeY);

	IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::Fill_WER(IPVM::Rect(outputImageCos), validRoi, 0, outputImageCos));
	IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::Fill_WER(IPVM::Rect(outputImageSin), validRoi, 0, outputImageSin));
	IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::Fill_WER(IPVM::Rect(outputAmplitude), validRoi, 0.f, outputAmplitude));
	IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::Fill_WER(IPVM::Rect(outputPhase), validRoi, NOISE_VALUE_32F, outputPhase));

	const uint16_t maxGray = (0x0001 << parameter.m_imageBitCount) - 1;
	const uint16_t halfGray = 0x0001 << (parameter.m_imageBitCount - 1);

	concurrency::parallel_for(int32_t(validRoi.m_top), validRoi.m_bottom,
		[&](int32_t y)
		{
			const auto* mem_fftBuffer = fftBuffer.GetMem(0, y);
			auto* mem_outputImageCos = outputImageCos.GetMem(0, y);
			auto* mem_outputImageSin = outputImageSin.GetMem(0, y);
			auto* mem_ooutputAmplitude = outputAmplitude.GetMem(0, y);
			auto* mem_outputPhase = outputPhase.GetMem(0, y);

			for (int32_t x = validRoi.m_left; x < validRoi.m_right; x++)
			{
				const float cos = parameter.m_fft_fringeReconstructionGain * mem_fftBuffer[x].m_x + halfGray + 0.5f;

				if (cos > maxGray)
				{
					mem_outputImageCos[x] = maxGray;
				}
				else if (cos < 0)
				{
					mem_outputImageCos[x] = 0;
				}
				else
				{
					mem_outputImageCos[x] = static_cast<uint16_t>(cos);
				}

				const float sin = parameter.m_fft_fringeReconstructionGain * mem_fftBuffer[x].m_y + halfGray + 0.5f;

				if (sin > maxGray)
				{
					mem_outputImageSin[x] = maxGray;
				}
				else if (sin < 0)
				{
					mem_outputImageSin[x] = 0;
				}
				else
				{
					mem_outputImageSin[x] = static_cast<uint16_t>(sin);
				}

				mem_ooutputAmplitude[x] = static_cast<float>(std::sqrt(mem_fftBuffer[x].m_x * mem_fftBuffer[x].m_x + mem_fftBuffer[x].m_y * mem_fftBuffer[x].m_y));
				mem_outputPhase[x] = std::atan2(mem_fftBuffer[x].m_y, mem_fftBuffer[x].m_x);
			}
		}
	);

	imageLogger.Push(_T("Output Amplitude"), outputAmplitude, IPVM::ColorMapIndex::Rainbow);
	imageLogger.Push(_T("Output Sin"), outputImageSin);
	imageLogger.Push(_T("Output Cos"), outputImageCos);
	imageLogger.Push(_T("Output Phase"), outputPhase, IPVM::ColorMapIndex::Rainbow);

	IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::Fill_WER(IPVM::Rect(phaseMap), validRoi, NOISE_VALUE_32F, phaseMap));

	IPVM_RUNTIME_CHECK(IPVM::SpatialPhaseUnwrapping().Unwrapping_SRFNCP(outputPhase, validRoi, NOISE_VALUE_32F, phaseMap));

	imageLogger.Push(_T("Output Phase Unwrapped"), phaseMap, IPVM::ColorMapIndex::Rainbow, true);

	return IPVM::Status::success;
}

IPVM::Status Process::MakeZmap(const Parameter& parameter, const IPVM::Image_32f_C1& phaseMap, IPVM::Image_32f_C1& zMap, ImageLogger& imageLogger)
{
	const auto validRoi = GetValidROI(parameter, phaseMap);

	IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::Fill_WER(IPVM::Rect(zMap), validRoi, NOISE_VALUE_32F, zMap));

	const double illuminationIncidence_theta_rad = M_PI * parameter.m_illuminationIncidence_theta_degree / 180.;
	const double verticalGratingPitch_um = parameter.m_illuminationGratingPitch_mm / cos(M_PI / 2 - illuminationIncidence_theta_rad) * 1000;

	concurrency::parallel_for(int32_t(validRoi.m_top), validRoi.m_bottom,
		[&](int32_t y)
		{
			const auto* mem_phaseMap = phaseMap.GetMem(0, y);
			auto* mem_zMap = zMap.GetMem(0, y);

			for (int32_t x = validRoi.m_left; x < validRoi.m_right; x++)
			{
				if (mem_phaseMap[x] != NOISE_VALUE_32F)
				{
					float nval = static_cast<float>(verticalGratingPitch_um * mem_phaseMap[x] / (2 * M_PI));
					mem_zMap[x] = nval;
				}
				else
				{
					mem_zMap[x] = NOISE_VALUE_32F;
				}
			}
		}
	);

	imageLogger.Push(_T("Output Z-Map (um)"), zMap, IPVM::ColorMapIndex::Rainbow, true);

	return IPVM::Status::success;
}

IPVM::Status Process::RemoveBackgroundShape(const Parameter& parameter, const IPVM::Image_32f_C1& zMap, ImageLogger& imageLogger)
{
	const auto imageSizeX = zMap.GetSizeX();
	const auto imageSizeY = zMap.GetSizeY();

	const auto validRoi = GetValidROI(parameter, zMap);

	std::vector<IPVM::Point_32f_C3> points;
	points.reserve(validRoi.Width() * validRoi.Height() / 16);

	const float imageCenterX = (imageSizeX - 1) * 0.5f;
	const float imageCenterY = (imageSizeY - 1) * 0.5f;

	for (int32_t y = validRoi.m_top; y < validRoi.m_bottom; y += 4)
	{
		const auto* mem_outputZmap = zMap.GetMem(0, y);

		for (int32_t x = validRoi.m_left; x < validRoi.m_right; x += 4)
		{
			if (mem_outputZmap[x] != NOISE_VALUE_32F)
			{
				points.emplace_back(static_cast<float>(x - imageCenterX), static_cast<float>(y - imageCenterY), mem_outputZmap[x]);
			}
		}
	}

	std::vector<double> coefs(IPVM::Polynomial::GetCoefCount(10));
	double* mem_coefs = &coefs[0];

	IPVM_RUNTIME_CHECK(IPVM::DataFitting::FitToNthOrderPolynomial3DSurface(points.size(), &points[0], 10, mem_coefs));

	IPVM::Image_32f_C1 outputZmap(imageSizeX, imageSizeY);

	IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::Fill_WER(IPVM::Rect(outputZmap), validRoi, NOISE_VALUE_32F, outputZmap));

	concurrency::parallel_for(int32_t(validRoi.m_top), validRoi.m_bottom,
		[&](int32_t y)
		{
			const auto* mem_zMap = zMap.GetMem(0, y);
			auto* mem_outputZmap = outputZmap.GetMem(0, y);

			for (int32_t x = validRoi.m_left; x < validRoi.m_right; x++)
			{
				if (mem_zMap[x] != NOISE_VALUE_32F)
				{
					mem_outputZmap[x] = static_cast<float>(mem_zMap[x] - IPVM::Polynomial::GetValue10th(static_cast<float>(x - imageCenterX), static_cast<float>(y - imageCenterY), mem_coefs));
				}
			}
		}
	);

	imageLogger.Push(_T("Output Z-Map Background Removed (um)"), outputZmap, IPVM::ColorMapIndex::Rainbow, true);

	return IPVM::Status::success;
}
