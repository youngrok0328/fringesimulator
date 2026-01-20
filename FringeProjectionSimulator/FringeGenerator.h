#pragma once

struct Parameter;

class FringeGenerator
{
public:
	static IPVM::Status Generate( Parameter& parameter, IPVM::Image_16u_C1 &dst, IPVM::Image_16u_C1& Hordst);
	
private:
	static IPVM::Status MakeShape(const Parameter& parameter, IPVM::Image_64f_C1& shapeImage);
	static IPVM::Status MakeShaperitheAngle(const Parameter& parameter, IPVM::Image_64f_C1& shapeImage);
	static IPVM::Status MakeSmoothSurfaceNormal(const Parameter& parameter, const IPVM::Image_64f_C1& shapeImage, IPVM::Image_64f_C3& surfaceNormal);
	static IPVM::Status MakeRoughSurfaceNormal(const Parameter& parameter, const IPVM::Image_64f_C3& smoothSurfaceNormal, IPVM::Image_64f_C3& surfaceNormal);
	static IPVM::Status MakeSurfaceReflectance(const Parameter& parameter, const IPVM::Image_64f_C3& surfaceNormal, IPVM::Image_64f_C1& surfaceReflectance);
	static IPVM::Status MakeIntensityImage(const Parameter& parameter, const IPVM::Image_64f_C1& surfaceReflectance, IPVM::Image_64f_C1& intensityImage);
	static IPVM::Status MakePhaseMap(const Parameter& parameter, const IPVM::Image_64f_C1& shapeImage, IPVM::Image_64f_C1& phaseMap);
	static IPVM::Status MakeModulationImage(const Parameter& parameter, const IPVM::Image_64f_C1& phaseMap, const IPVM::Image_64f_C1& intensityImage, IPVM::Image_16u_C1& modulationImage);
	static IPVM::Status MakeNoiseImage(const Parameter& parameter, IPVM::Image_16u_C1& modulationImage);
	static IPVM::Status GenerateHor(Parameter& parameter, IPVM::Image_16u_C1& dst);
};

