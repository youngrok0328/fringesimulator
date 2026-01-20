#include "pch.h"
#include "FringeGenerator.h"
#include "Parameter.h"

#include "Algorithm/DataFitting.h"
#include "Algorithm/FastFourierTransform1D.h"
#include "Algorithm/ImageProcessing.h"
#include "Base/Status.h"
#include "Types/Image_32f_C1.h"
#include "Types/Image_64f_C1.h"
#include "Types/Image_64f_C3.h"
#include "Types/Image_16u_C1.h"
#include "Types/PlaneEq_64f.h"
#include "Types/Point_64f_C2.h"
#include "Types/Point_64f_C3.h"

#include <ppl.h>
#include <random>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IPVM::Status FringeGenerator::Generate( Parameter& parameter, IPVM::Image_16u_C1& dst, IPVM::Image_16u_C1& Hordst)
{
	const auto fftSizeX = IPVM::FastFourierTransform1D().GetFFTLength(parameter.m_imageSizeX);
	const auto fftSizeY = IPVM::FastFourierTransform1D().GetFFTLength(parameter.m_imageSizeY);

	parameter.m_illuminationIncidence_phi_degree = 0; // 기본값은 0 (수직) 

	IPVM::Image_16u_C1 modulationImage(fftSizeX, fftSizeY);

	if constexpr(1)
	{
		IPVM::Image_64f_C1 shapeImage(fftSizeX, fftSizeY);
		IPVM_RUNTIME_CHECK(MakeShape(parameter, shapeImage));

		IPVM::Image_64f_C1 surfaceReflectance(fftSizeX, fftSizeY);

		if constexpr (1)
		{
			IPVM::Image_64f_C3 smoothSurfaceNormal(fftSizeX, fftSizeY);
			IPVM_RUNTIME_CHECK(MakeSmoothSurfaceNormal(parameter, shapeImage, smoothSurfaceNormal));

			IPVM::Image_64f_C3 roughSurfaceNormal(fftSizeX, fftSizeY);
			IPVM_RUNTIME_CHECK(MakeRoughSurfaceNormal(parameter, smoothSurfaceNormal, roughSurfaceNormal));

			IPVM_RUNTIME_CHECK(MakeSurfaceReflectance(parameter, roughSurfaceNormal, surfaceReflectance));
		}

		if constexpr (1)
		{
			IPVM::Image_64f_C1 intensityImage(fftSizeX, fftSizeY);
			IPVM_RUNTIME_CHECK(MakeIntensityImage(parameter, surfaceReflectance, intensityImage));

			IPVM::Image_64f_C1 phaseMap(fftSizeX, fftSizeY);
			IPVM_RUNTIME_CHECK(MakePhaseMap(parameter, shapeImage, phaseMap));

			IPVM_RUNTIME_CHECK(MakeModulationImage(parameter, phaseMap, intensityImage, modulationImage));
		}
	}

	if (parameter.m_pointSpreadFunctionSigma > 0. && parameter.m_pointSpreadFunctionKernelSize > 3)
	{
		IPVM::Image_16u_C1 lowPassImage(fftSizeX, fftSizeY);

		IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::FilterGaussTwoPara(modulationImage, IPVM::Rect(modulationImage), static_cast<float>(parameter.m_pointSpreadFunctionSigma), parameter.m_pointSpreadFunctionKernelSize, true, lowPassImage));

		modulationImage = lowPassImage;
	}

	IPVM_RUNTIME_CHECK(MakeNoiseImage(parameter, modulationImage));

	dst.Create(fftSizeX, fftSizeY);
	dst.FillZero();

	const auto offsetX = (fftSizeX - parameter.m_imageSizeX) / 2;
	const auto offsetY = (fftSizeY - parameter.m_imageSizeY) / 2;

	const IPVM::Rect innerArea(offsetX, offsetY, offsetX + parameter.m_imageSizeX, offsetY + parameter.m_imageSizeY);

	IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::Copy(modulationImage, innerArea, dst));

	GenerateHor(parameter, Hordst);

	return IPVM::Status::success;
}

IPVM::Status FringeGenerator::GenerateHor(Parameter& parameter, IPVM::Image_16u_C1& dst)
{
	const auto fftSizeX = IPVM::FastFourierTransform1D().GetFFTLength(parameter.m_imageSizeX);
	const auto fftSizeY = IPVM::FastFourierTransform1D().GetFFTLength(parameter.m_imageSizeY);

	IPVM::Image_16u_C1 modulationImage(fftSizeX, fftSizeY);

	parameter.m_illuminationIncidence_phi_degree = 90; // 기본값은 0 (수직) , 수평영상을 생성하기 위해 90도로 변경
	
	if constexpr (1)
	{
		IPVM::Image_64f_C1 shapeImage(fftSizeX, fftSizeY);
		IPVM_RUNTIME_CHECK(MakeShape(parameter, shapeImage));

		IPVM::Image_64f_C1 surfaceReflectance(fftSizeX, fftSizeY);

		if constexpr (1)
		{
			IPVM::Image_64f_C3 smoothSurfaceNormal(fftSizeX, fftSizeY);
			IPVM_RUNTIME_CHECK(MakeSmoothSurfaceNormal(parameter, shapeImage, smoothSurfaceNormal));

			IPVM::Image_64f_C3 roughSurfaceNormal(fftSizeX, fftSizeY);
			IPVM_RUNTIME_CHECK(MakeRoughSurfaceNormal(parameter, smoothSurfaceNormal, roughSurfaceNormal));

			IPVM_RUNTIME_CHECK(MakeSurfaceReflectance(parameter, roughSurfaceNormal, surfaceReflectance));
		}

		if constexpr (1)
		{
			IPVM::Image_64f_C1 intensityImage(fftSizeX, fftSizeY);
			IPVM_RUNTIME_CHECK(MakeIntensityImage(parameter, surfaceReflectance, intensityImage));

			IPVM::Image_64f_C1 phaseMap(fftSizeX, fftSizeY);
			IPVM_RUNTIME_CHECK(MakePhaseMap(parameter, shapeImage, phaseMap));

			IPVM_RUNTIME_CHECK(MakeModulationImage(parameter, phaseMap, intensityImage, modulationImage));
		}
	}

	if (parameter.m_pointSpreadFunctionSigma > 0. && parameter.m_pointSpreadFunctionKernelSize > 3)
	{
		IPVM::Image_16u_C1 lowPassImage(fftSizeX, fftSizeY);

		IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::FilterGaussTwoPara(modulationImage, IPVM::Rect(modulationImage), static_cast<float>(parameter.m_pointSpreadFunctionSigma), parameter.m_pointSpreadFunctionKernelSize, true, lowPassImage));

		modulationImage = lowPassImage;
	}

	IPVM_RUNTIME_CHECK(MakeNoiseImage(parameter, modulationImage));

	dst.Create(fftSizeX, fftSizeY);
	dst.FillZero();

	const auto offsetX = (fftSizeX - parameter.m_imageSizeX) / 2;
	const auto offsetY = (fftSizeY - parameter.m_imageSizeY) / 2;

	const IPVM::Rect innerArea(offsetX, offsetY, offsetX + parameter.m_imageSizeX, offsetY + parameter.m_imageSizeY);

	IPVM_RUNTIME_CHECK(IPVM::ImageProcessing::Copy(modulationImage, innerArea, dst));

	return IPVM::Status::success;
}

IPVM::Status FringeGenerator::MakeShape(const Parameter& parameter, IPVM::Image_64f_C1& shapeImage)
{
	const int32_t imageSizeX = shapeImage.GetSizeX();
	const int32_t imageSizeY = shapeImage.GetSizeY();

	const double imageCenterX_mm = 0.5 * (imageSizeX - 1) * parameter.m_px2mm;
	const double imageCenterY_mm = 0.5 * (imageSizeY - 1) * parameter.m_px2mm;

	const double twoSigmaSquare_mm2 = 0.25 * parameter.m_gaussianShapeFWHM_mm * parameter.m_gaussianShapeFWHM_mm / std::log(2);

	concurrency::parallel_for(int32_t(0), imageSizeY, 
		[&](int32_t y)
		{
			auto* mem = shapeImage.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				const double x_mm = x * parameter.m_px2mm - imageCenterX_mm;
				const double y_mm = imageCenterY_mm - y * parameter.m_px2mm;

				const double dx_mm = x_mm - parameter.m_gaussainShapePositionX_mm;
				const double dy_mm = y_mm - parameter.m_gaussainShapePositionY_mm;

				mem[x] = parameter.m_gaussianShapeDepth_mm * std::exp(-(dx_mm * dx_mm + dy_mm * dy_mm )/ twoSigmaSquare_mm2)
					+ parameter.m_backgroundShape_xx * x_mm * x_mm
					+ parameter.m_backgroundShape_xy * x_mm * y_mm
					+ parameter.m_backgroundShape_yy * y_mm * y_mm
					+ parameter.m_backgroundShape_x * x_mm
					+ parameter.m_backgroundShape_y * y_mm
					+ parameter.m_backgroundShape_z0;
			}
		}
	);

	return IPVM::Status::success;
}

IPVM::Status FringeGenerator::MakeShaperitheAngle(const Parameter& parameter, IPVM::Image_64f_C1& shapeImage)
{
	return IPVM::Status::success;
}

IPVM::Status FringeGenerator::MakeSmoothSurfaceNormal(const Parameter& parameter, const IPVM::Image_64f_C1& shapeImage, IPVM::Image_64f_C3& surfaceNormal)
{
	const int32_t imageSizeX = shapeImage.GetSizeX();
	const int32_t imageSizeY = shapeImage.GetSizeY();

	for (int32_t y = 0; y < imageSizeY; y++)
	{
		auto* dst = surfaceNormal.GetMem(0, y);

		for (int32_t x = 0; x < imageSizeX; x++)
		{
			dst[x].m_x = 0.f;
			dst[x].m_y = 0.f;
			dst[x].m_z = 1.f;
		}
	}

	concurrency::parallel_for(int32_t(0), imageSizeY,
		[&](int32_t y)
		{
			IPVM::Point_64f_C3 buffer[9];

			for (int32_t yy = 0; yy < 3; yy++)
			{
				for (int32_t xx = 0; xx < 3; xx++)
				{
					buffer[3 * yy + xx].m_x = (xx - 1) * parameter.m_px2mm;
					buffer[3 * yy + xx].m_y = (1 - yy) * parameter.m_px2mm;
				}
			}

			const auto* prev = shapeImage.GetMem(0, y - 1);
			const auto* curr = shapeImage.GetMem(0, y);
			const auto* next = shapeImage.GetMem(0, y + 1);

			auto* dst = surfaceNormal.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				int32_t count = 0;

				if (y > 0)
				{
					if (x > 0)
					{
						buffer[count++].m_z = prev[x - 1];
					}

					buffer[count++].m_z = prev[x];

					if (x < imageSizeX - 1)
					{
						buffer[count++].m_z = prev[x + 1];
					}
				}

				if (x > 0)
				{
					buffer[count++].m_z = curr[x - 1];
				}

				buffer[count++].m_z = curr[x];

				if (x < imageSizeX - 1)
				{
					buffer[count++].m_z = curr[x + 1];
				}

				if (y < imageSizeY - 1)
				{
					if (x > 0)
					{
						buffer[count++].m_z = next[x - 1];
					}

					buffer[count++].m_z = next[x];

					if (x < imageSizeX - 1)
					{
						buffer[count++].m_z = next[x + 1];
					}
				}

				IPVM::PlaneEq_64f planeEq;

				if (IPVM::Status::success == IPVM::DataFitting::FitToPlane(count, buffer, planeEq))
				{
					const double mag = std::sqrt(planeEq.m_a * planeEq.m_a + planeEq.m_b * planeEq.m_b + planeEq.m_c * planeEq.m_c);

					// 원래 형상으로부터 계산되는 표면 법선 벡터
					dst[x].m_x = planeEq.m_a / mag;
					dst[x].m_y = planeEq.m_b / mag;
					dst[x].m_z = planeEq.m_c / mag;
				}
			}
		}
	);

	return IPVM::Status::success;
}

// 랜덤 함수에는 일부러 병렬화 안함...
IPVM::Status FringeGenerator::MakeRoughSurfaceNormal(const Parameter& parameter, const IPVM::Image_64f_C3& smoothSurfaceNormal, IPVM::Image_64f_C3& surfaceNormal)
{
	const int32_t imageSizeX = smoothSurfaceNormal.GetSizeX();
	const int32_t imageSizeY = smoothSurfaceNormal.GetSizeY();

	const double surfaceNormalRandomLimitX_rad = M_PI * fabs(parameter.m_surfaceNormalRandomLimitX_degree) / 180.;
	const double surfaceNormalRandomLimitY_rad = M_PI * fabs(parameter.m_surfaceNormalRandomLimitY_degree) / 180.;

	if (surfaceNormalRandomLimitX_rad > 0. && surfaceNormalRandomLimitY_rad > 0.)
	{
		std::mt19937 genX(1729);
		std::normal_distribution<double> randomDeltaX(0, surfaceNormalRandomLimitX_rad);

		std::mt19937 genY(1730);
		std::normal_distribution<double> randomDeltaY(0, surfaceNormalRandomLimitY_rad);

		for (int32_t y = 0; y < imageSizeY; y++)
		{
			const auto* src = smoothSurfaceNormal.GetMem(0, y);
			auto* dst = surfaceNormal.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				// 원래 형상으로부터 계산된 표면 법선 벡터
				const double n_x = src[x].m_x;
				const double n_y = src[x].m_y;
				const double n_z = src[x].m_z;

				// 표면을 울퉁불퉁하게 만들기 위해 법선을 랜덤하게 기울기를 더 준다.

				const double delta_x = std::tan(randomDeltaX(genX));
				const double delta_y = std::tan(randomDeltaY(genY));

				const double n2_x = n_x + delta_x * n_z / std::sqrt(n_x * n_x + n_z * n_z);
				const double n2_y = n_y + delta_y * n_z / std::sqrt(n_y * n_y + n_z * n_z);
				const double n2_z = n_z - (n_x * n2_x + n_y * n2_y) / n_z;

				const double mag2 = std::sqrt(n2_x * n2_x + n2_y * n2_y + n2_z * n2_z);

				dst[x].m_x = n2_x / mag2;
				dst[x].m_y = n2_y / mag2;
				dst[x].m_z = n2_z / mag2;
			}
		}
	}
	else if (surfaceNormalRandomLimitX_rad > 0.)
	{
		std::mt19937 genX(1729);
		std::normal_distribution<double> randomDeltaX(0, surfaceNormalRandomLimitX_rad);

		for (int32_t y = 0; y < imageSizeY; y++)
		{
			const auto* src = smoothSurfaceNormal.GetMem(0, y);
			auto* dst = surfaceNormal.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				// 원래 형상으로부터 계산된 표면 법선 벡터
				const double n_x = src[x].m_x;
				const double n_y = src[x].m_y;
				const double n_z = src[x].m_z;

				// 표면을 울퉁불퉁하게 만들기 위해 법선을 랜덤하게 기울기를 더 준다.

				const double delta_x = std::tan(randomDeltaX(genX));

				const double n2_x = n_x + delta_x * n_z / std::sqrt(n_x * n_x + n_z * n_z);
				const double n2_y = n_y;
				const double n2_z = n_z - (n_x * n2_x + n_y * n2_y) / n_z;

				const double mag2 = std::sqrt(n2_x * n2_x + n2_y * n2_y + n2_z * n2_z);

				dst[x].m_x = n2_x / mag2;
				dst[x].m_y = n2_y / mag2;
				dst[x].m_z = n2_z / mag2;
			}
		}
	}
	else if (surfaceNormalRandomLimitY_rad > 0.)
	{
		std::mt19937 genY(1730);
		std::normal_distribution<double> randomDeltaY(0, surfaceNormalRandomLimitY_rad);

		for (int32_t y = 0; y < imageSizeY; y++)
		{
			const auto* src = smoothSurfaceNormal.GetMem(0, y);
			auto* dst = surfaceNormal.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				// 원래 형상으로부터 계산된 표면 법선 벡터
				const double n_x = src[x].m_x;
				const double n_y = src[x].m_y;
				const double n_z = src[x].m_z;

				// 표면을 울퉁불퉁하게 만들기 위해 법선을 랜덤하게 기울기를 더 준다.

				const double delta_y = std::tan(randomDeltaY(genY));

				const double n2_x = n_x;
				const double n2_y = n_y + delta_y * n_z / std::sqrt(n_y * n_y + n_z * n_z);
				const double n2_z = n_z - (n_x * n2_x + n_y * n2_y) / n_z;

				const double mag2 = std::sqrt(n2_x * n2_x + n2_y * n2_y + n2_z * n2_z);

				dst[x].m_x = n2_x / mag2;
				dst[x].m_y = n2_y / mag2;
				dst[x].m_z = n2_z / mag2;
			}
		}
	}
	else
	{
		for (int32_t y = 0; y < imageSizeY; y++)
		{
			const auto* src = smoothSurfaceNormal.GetMem(0, y);
			auto* dst = surfaceNormal.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				// 원래 형상으로부터 계산된 표면 법선 벡터
				const double n_x = src[x].m_x;
				const double n_y = src[x].m_y;
				const double n_z = src[x].m_z;

				// 표면을 울퉁불퉁하게 만들기 위해 법선을 랜덤하게 기울기를 더 준다.
				const double n2_x = n_x;
				const double n2_y = n_y;
				const double n2_z = n_z - (n_x * n2_x + n_y * n2_y) / n_z;

				const double mag2 = std::sqrt(n2_x * n2_x + n2_y * n2_y + n2_z * n2_z);

				dst[x].m_x = n2_x / mag2;
				dst[x].m_y = n2_y / mag2;
				dst[x].m_z = n2_z / mag2;
			}
		}
	}

	return IPVM::Status::success;
}

IPVM::Status FringeGenerator::MakeSurfaceReflectance(const Parameter& parameter, const IPVM::Image_64f_C3& surfaceNormal, IPVM::Image_64f_C1& surfaceReflectance)
{
	const int32_t imageSizeX = surfaceNormal.GetSizeX();
	const int32_t imageSizeY = surfaceNormal.GetSizeY();

	const double illuminationIncidence_theta_rad = M_PI * parameter.m_illuminationIncidence_theta_degree / 180.;
	const double illuminationIncidence_phi_rad = M_PI * parameter.m_illuminationIncidence_phi_degree / 180.;

	const IPVM::Point_64f_C3 illuminationVector(
		std::sin(illuminationIncidence_theta_rad) * std::cos(illuminationIncidence_phi_rad),
		std::sin(illuminationIncidence_theta_rad) * std::sin(illuminationIncidence_phi_rad),
		std::cos(illuminationIncidence_theta_rad));

	const double twoReflectionDistributionSqr_rad = 2 * parameter.m_reflectionDistribution_rad * parameter.m_reflectionDistribution_rad;

	const IPVM::Point_64f_C3 cameraVector(0, 0, 1);

	concurrency::parallel_for(int32_t(0), imageSizeY,
		[&](int32_t y)
		{
			const auto* normalVector = surfaceNormal.GetMem(0, y);
			auto* reflectance = surfaceReflectance.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				const auto& v = normalVector[x];
				const auto& u = illuminationVector;

				// 하우스홀더 반사 공식을 사용
				// u2 = u - 2 * (ut dot v) / (vt dot v) * v; 

				const double ut_dot_v = u.m_x * v.m_x + u.m_y * v.m_y + u.m_z * v.m_z;
				const double vt_dot_v = v.m_x * v.m_x + v.m_y * v.m_y + v.m_z * v.m_z;
				const double coef = 2 * ut_dot_v / vt_dot_v;

				const IPVM::Point_64f_C3 u2(u.m_x - coef * v.m_x, u.m_y - coef * v.m_y, u.m_z - coef * v.m_z);

				// 실제 반사벡터 구하기
				const IPVM::Point_64f_C3 u3(-u2.m_x, -u2.m_y, -u2.m_z);

				const auto& c = cameraVector;

				// 텔레센트릭으로 가정하여...
				const double surfaceToCameraAngle_rad = std::acos(c.m_x * u3.m_x + c.m_y * u3.m_y + c.m_z * u3.m_z);

				// 원래는 Bi-Directional Reflection Distribution 으로 해야하는데 모델링이 복잡하므로,
				// 어떤 각도로 입사하든지 상관없이 전반사각 기준으로 동일한 가우시안 분포를 가지는 것으로 한다.
				const double lambertianReflectance = std::exp(-surfaceToCameraAngle_rad * surfaceToCameraAngle_rad / twoReflectionDistributionSqr_rad);

				// 조명 입사각에 의한 밝기 저하 고려
				const double attenuationRatio = v.m_x * u.m_x + v.m_y * u.m_y + v.m_z * u.m_z;

				reflectance[x] = lambertianReflectance * attenuationRatio;
			}
		}
	);

	return IPVM::Status::success;
}

IPVM::Status FringeGenerator::MakeIntensityImage(const Parameter& parameter, const IPVM::Image_64f_C1& surfaceReflectance, IPVM::Image_64f_C1& intensityImage)
{
	const int32_t imageSizeX = surfaceReflectance.GetSizeX();
	const int32_t imageSizeY = surfaceReflectance.GetSizeY();

	concurrency::parallel_for(int32_t(0), imageSizeY,
		[&](int32_t y)
		{
			const auto* reflectance = surfaceReflectance.GetMem(0, y);
			auto* pixelIntensity = intensityImage.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				pixelIntensity[x] = parameter.m_illuminationIntensity_gv * reflectance[x];
			}
		}
	);

	return IPVM::Status::success;
}

IPVM::Status FringeGenerator::MakePhaseMap(const Parameter& parameter, const IPVM::Image_64f_C1& shapeImage, IPVM::Image_64f_C1& phaseMap)
{
	const int32_t imageSizeX = shapeImage.GetSizeX();
	const int32_t imageSizeY = shapeImage.GetSizeY();

	const double illuminationIncidence_theta_rad = M_PI * parameter.m_illuminationIncidence_theta_degree / 180.;
	const double illuminationIncidence_phi_rad = M_PI * parameter.m_illuminationIncidence_phi_degree / 180.;
	const double phaseOffset_rad = M_PI * parameter.m_illuminationGratingPhaseOffset_deg / 180.;

	const double lateralGratingPitch_mm = parameter.m_illuminationGratingPitch_mm / cos(illuminationIncidence_theta_rad);
	const double verticalGratingPitch_mm = parameter.m_illuminationGratingPitch_mm / cos(M_PI / 2 - illuminationIncidence_theta_rad);

	const IPVM::Point_64f_C3 illuminationVector(
		std::sin(illuminationIncidence_theta_rad) * std::cos(illuminationIncidence_phi_rad),
		std::sin(illuminationIncidence_theta_rad) * std::sin(illuminationIncidence_phi_rad),
		std::cos(illuminationIncidence_theta_rad));

	const double referenceLineNormalVectorMagnitude = sqrt(illuminationVector.m_x * illuminationVector.m_x + illuminationVector.m_y * illuminationVector.m_y);
	const IPVM::Point_64f_C2 referenceLineNormalVector(illuminationVector.m_x / referenceLineNormalVectorMagnitude, illuminationVector.m_y / referenceLineNormalVectorMagnitude);

	concurrency::parallel_for(int32_t(0), imageSizeY,
		[&](int32_t y)
		{
			const auto* src = shapeImage.GetMem(0, y);
			auto* dst = phaseMap.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				const double x_mm = +x * parameter.m_px2mm;
				const double y_mm = -y * parameter.m_px2mm;
				const double z_mm = src[x];

				const double toReferenceLineDistance = x_mm * referenceLineNormalVector.m_x + y_mm * referenceLineNormalVector.m_y;

				dst[x] = 2 * M_PI * (toReferenceLineDistance / lateralGratingPitch_mm + z_mm / verticalGratingPitch_mm) + phaseOffset_rad;
			}
		}
	);

	return IPVM::Status::success;
}

IPVM::Status FringeGenerator::MakeModulationImage(const Parameter& parameter, const IPVM::Image_64f_C1& phaseMap, const IPVM::Image_64f_C1& intensityImage, IPVM::Image_16u_C1& modulationImage)
{
	const int32_t imageSizeX = phaseMap.GetSizeX();
	const int32_t imageSizeY = phaseMap.GetSizeY();

	const uint16_t maxGray = (0x0001 << parameter.m_imageBitCount) - 1;

	concurrency::parallel_for(int32_t(0), imageSizeY,
		[&](int32_t y)
		{
			const auto* phase = phaseMap.GetMem(0, y);
			const auto* intensity = intensityImage.GetMem(0, y);

			auto* dst = modulationImage.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				const double grayValue = (0.5 * std::cos(phase[x]) + 0.5) * intensity[x] + parameter.m_modulationIntensityOffset_gv;

				if (grayValue > maxGray)
				{
					dst[x] = maxGray;
				}
				else if (grayValue <= 0.)
				{
					dst[x] = 0;
				}
				else
				{
					dst[x] = uint16_t(grayValue + 0.5f);
				}
			}
		}
	);

	return IPVM::Status::success;
}

// 랜덤 함수에는 일부러 병렬화 안함...
IPVM::Status FringeGenerator::MakeNoiseImage(const Parameter& parameter, IPVM::Image_16u_C1& modulationImage)
{
	const int32_t imageSizeX = modulationImage.GetSizeX();
	const int32_t imageSizeY = modulationImage.GetSizeY();

	const uint16_t maxGray = (0x0001 << parameter.m_imageBitCount) - 1;

	 std::random_device randomGenerator;
	//std::mt19937 randomGenerator(1729);

	if (parameter.m_poissonNoiseLimit_ratio > 0. && parameter.m_gaussianNoiseSigma_gv > 0.)
	{
		std::vector<std::poisson_distribution<int32_t>> poissonDistributions;
		std::normal_distribution<float> gaussDistribution(0, static_cast<float>(parameter.m_gaussianNoiseSigma_gv));

		poissonDistributions.emplace_back(0.1);

		for (double mean = 1; mean <= maxGray; mean++)
		{
			poissonDistributions.emplace_back(mean);
		}

		for (int32_t y = 0; y < imageSizeY; y++)
		{
			auto* dst = modulationImage.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				const auto orig = dst[x];

				const auto poissonNoiseAdded = poissonDistributions[orig](randomGenerator);
				const auto poissonNoise = parameter.m_poissonNoiseLimit_ratio * (poissonNoiseAdded - orig);

				const auto gaussianNoise = gaussDistribution(randomGenerator);

				const auto grayValue = orig + poissonNoise + gaussianNoise;

				if (grayValue > maxGray)
				{
					dst[x] = maxGray;
				}
				else if (grayValue <= 0.)
				{
					dst[x] = 0;
				}
				else
				{
					dst[x] = static_cast<uint16_t>(grayValue);
				}
			}
		}
	}
	else if (parameter.m_poissonNoiseLimit_ratio > 0.)
	{
		std::vector<std::poisson_distribution<int32_t>> poissonDistributions;

		poissonDistributions.emplace_back(0.1);

		for (double mean = 1; mean <= maxGray; mean++)
		{
			poissonDistributions.emplace_back(mean);
		}

		for (int32_t y = 0; y < imageSizeY; y++)
		{
			auto* dst = modulationImage.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				const auto orig = dst[x];

				const auto poissonNoiseAdded = poissonDistributions[orig](randomGenerator);
				const auto poissonNoise = parameter.m_poissonNoiseLimit_ratio * (poissonNoiseAdded - orig);

				const auto grayValue = orig + poissonNoise;

				if (grayValue > maxGray)
				{
					dst[x] = maxGray;
				}
				else if (grayValue <= 0.)
				{
					dst[x] = 0;
				}
				else
				{
					dst[x] = static_cast<uint16_t>(grayValue + 0.5);
				}
			}
		}
	}
	else if (parameter.m_gaussianNoiseSigma_gv > 0.)
	{
		std::normal_distribution<float> gaussDistribution(0, static_cast<float>(parameter.m_gaussianNoiseSigma_gv));

		for (int32_t y = 0; y < imageSizeY; y++)
		{
			auto* dst = modulationImage.GetMem(0, y);

			for (int32_t x = 0; x < imageSizeX; x++)
			{
				const auto orig = dst[x];
				const auto gaussianNoise = gaussDistribution(randomGenerator);

				const auto grayValue = orig + gaussianNoise;

				if (grayValue > maxGray)
				{
					dst[x] = maxGray;
				}
				else if (grayValue <= 0)
				{
					dst[x] = 0;
				}
				else
				{
					dst[x] = static_cast<uint16_t>(grayValue);
				}
			}
		}
	}

	return IPVM::Status::success;
}
