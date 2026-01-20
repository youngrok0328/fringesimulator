#pragma once

#include <vector>
#include <tuple>

class ImageLogger
{
public:
	ImageLogger();
	~ImageLogger();

	void Reset();

	void Push(LPCTSTR title, IPVM::Image_16u_C1& image);
	void Push(LPCTSTR title, IPVM::Image_32f_C1& image, IPVM::ColorMapIndex colorMapIndex, const bool useErrorProfile = false);

	int32_t GetCount() const;

	LPCTSTR GetTitle(int32_t index) const;
	IPVM::Image* GetImage(int32_t index) const;
	IPVM::ColorMapIndex GetColorMapIndex(int32_t index) const;
	bool GetUseErrorProfile(int32_t index) const;

private:
	typedef std::tuple<CString, IPVM::Image*, IPVM::ColorMapIndex, bool> ElementType;

	std::vector<ElementType> m_items;
};

