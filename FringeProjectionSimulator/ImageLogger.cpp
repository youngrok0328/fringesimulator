#include "pch.h"
#include "ImageLogger.h"

#include "Algorithm/ImageProcessing.h"
#include "Base/ColorMapIndex.h"
#include "Types/Image.h"
#include "Types/Image_16u_C1.h"
#include "Types/Image_32f_C1.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

ImageLogger::ImageLogger()
{

}

ImageLogger::~ImageLogger()
{
	Reset();
}

void ImageLogger::Reset()
{
	for (auto &item : m_items)
	{
		delete std::get<1>(item);
	}

	m_items.clear();
}

void ImageLogger::Push(LPCTSTR title, IPVM::Image_16u_C1& image)
{
	auto logImage = new IPVM::Image_16u_C1(image.GetSizeX(), image.GetSizeY());

	IPVM::ImageProcessing::Copy(image, IPVM::Rect(image), *logImage);
	
	m_items.emplace_back(title, logImage, IPVM::ColorMapIndex::Black_White, false);
}

void ImageLogger::Push(LPCTSTR title, IPVM::Image_32f_C1& image, IPVM::ColorMapIndex colorMapIndex, const bool useErrorProfile)
{
	auto logImage = new IPVM::Image_32f_C1(image.GetSizeX(), image.GetSizeY());

	IPVM::ImageProcessing::Copy(image, IPVM::Rect(image), *logImage);

	m_items.emplace_back(title, logImage, colorMapIndex, useErrorProfile);
}

int32_t ImageLogger::GetCount() const
{
	return static_cast<int32_t>(m_items.size());
}

LPCTSTR ImageLogger::GetTitle(int32_t index) const
{
	if (index >= 0 && index < m_items.size())
	{
		return std::get<0>(m_items[index]);
	}
	else
	{
		return nullptr;
	}
}

IPVM::Image* ImageLogger::GetImage(int32_t index) const
{
	if (index >= 0 && index < m_items.size())
	{
		return std::get<1>(m_items[index]);
	}
	else
	{
		return nullptr;
	}
}

IPVM::ColorMapIndex ImageLogger::GetColorMapIndex(int32_t index) const
{
	if (index >= 0 && index < m_items.size())
	{
		return std::get<2>(m_items[index]);
	}
	else
	{
		return IPVM::ColorMapIndex::Black_White;
	}
}

bool ImageLogger::GetUseErrorProfile(int32_t index) const
{
	if (index >= 0 && index < m_items.size())
	{
		return std::get<3>(m_items[index]);
	}
	else
	{
		return false;
	}
}
