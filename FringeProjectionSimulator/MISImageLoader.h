#pragma once

class MISImageLoader
{
public:
	static int32_t GetFileVersion(LPCTSTR pathName);
	static bool Load_V3(LPCTSTR pathName, IPVM::Image_16u_C1& horImage, IPVM::Image_16u_C1& verImage);
	static bool Save_V3(LPCTSTR pathName, IPVM::Image_16u_C1& first_image, IPVM::Image_16u_C1& second_image, bool is_dual_shot);
	static bool Save_V4(LPCTSTR pathName, IPVM::Image_16u_C1& first_image, IPVM::Image_16u_C1& second_image, bool is_dual_shot, int nFovNum);
};

