#include "pch.h"
#include "MISImageLoader.h"

#include "Types/Image_8u_C1.h"
#include "Types/Image_16u_C1.h"

#include "Widget/AsyncProgress.h"
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define FILE_EXTENSION_TAG_V1	"ICBS_IMAGE_BITMAP_EXTENSION_{ACF1584F-DF57-4197-8480-491AC9DAF767}"
#define FILE_EXTENSION_TAG_V2	"ICBS_IMAGE_BITMAP_EXTENSION_{06AE4B14-2A3C-411C-8C7C-7F36D507FB07}"
#define FILE_EXTENSION_TAG_V3	"ICBS_IMAGE_BITMAP_EXTENSION_{DE50AB3B-FF2D-4F73-8149-7206B7EA8944}"

const long FILE_EXTENSION_TAG_LENGTH = sizeof(FILE_EXTENSION_TAG_V1);

int32_t MISImageLoader::GetFileVersion(LPCTSTR pathName)
{
	CFile file;

	if (!file.Open(pathName, CFile::modeRead | CFile::shareDenyWrite)) return -1;

	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	RGBQUAD palette[256];

	auto fileSize = file.GetLength();

	if (fileSize <= sizeof(bmfh) + sizeof(bmih) + sizeof(palette)) return -1;

	file.Read(&bmfh, sizeof(bmfh));
	file.Read(&bmih, sizeof(bmih));
	file.Read(&palette, sizeof(palette));

	if (bmfh.bfType != 0x4d42) return -1;

	file.Seek(bmih.biSizeImage, CFile::current);

	// 추가 정보를 위한 헤더
	char fileTag[FILE_EXTENSION_TAG_LENGTH];

	if (fileSize - file.GetPosition() < FILE_EXTENSION_TAG_LENGTH) return -1;

	file.Read(fileTag, FILE_EXTENSION_TAG_LENGTH);
	file.Close();

	if (::strcmp(fileTag, FILE_EXTENSION_TAG_V1) == 0) return 1;
	if (::strcmp(fileTag, FILE_EXTENSION_TAG_V2) == 0) return 2;
	if (::strcmp(fileTag, FILE_EXTENSION_TAG_V3) == 0) return 3;

	return -1;
}

struct ExtensionInfoHeader
{
	// 이미지 버퍼 할당을 위한 기본 정보
	int32_t fovIndex;
	int32_t imageAcquisitionMode;
	int32_t imageSizeX;
	int32_t imageSizeY;
};

bool MISImageLoader::Load_V3(LPCTSTR pathName, IPVM::Image_16u_C1& horImage, IPVM::Image_16u_C1& verImage)
{
	CFile file;

	if (!file.Open(pathName, CFile::modeRead | CFile::shareDenyWrite))
	{
		::AfxMessageBox(_T("Cannot open file!"), MB_OK | MB_ICONERROR);

		return false;
	}

	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	RGBQUAD palette[256];

	auto fileSize = file.GetLength();

	if (fileSize <= sizeof(bmfh) + sizeof(bmih) + sizeof(palette))
	{
		::AfxMessageBox(_T("File size is too small!"), MB_OK | MB_ICONERROR);
		return false;
	}

	file.Read(&bmfh, sizeof(bmfh));
	file.Read(&bmih, sizeof(bmih));
	file.Read(&palette, sizeof(palette));

	if (bmfh.bfType != 0x4d42)
	{
		::AfxMessageBox(_T("Cannot find bitmap info!"), MB_OK | MB_ICONERROR);
		return false;
	}

	// 첫번째 이미지는 상위 비트와 하위 비트를 나눠 저장되어 있다
	IPVM::Image_8u_C1 firstImage_Low(bmih.biWidth, abs(bmih.biHeight));
	IPVM::Image_8u_C1 firstImage_High(bmih.biWidth, abs(bmih.biHeight));

	file.Read(firstImage_High.GetMem(), bmih.biSizeImage);

	// 추가 정보를 위한 헤더
	if (fileSize - file.GetPosition() < FILE_EXTENSION_TAG_LENGTH)
	{
		::AfxMessageBox(_T("No extension tag!"), MB_OK | MB_ICONERROR);
		return false;
	}

	file.Seek(FILE_EXTENSION_TAG_LENGTH, CFile::current);
	file.Read(firstImage_Low.GetMem(), bmih.biSizeImage);

	if (fileSize - file.GetPosition() < sizeof(ExtensionInfoHeader))
	{
		::AfxMessageBox(_T("No extension information!"), MB_OK | MB_ICONERROR);
		return false;
	}

	ExtensionInfoHeader eih;
	file.Read(&eih, sizeof(eih));

	ULONGLONG extensionImageTotalBytes = 0;

	extensionImageTotalBytes += IPVM::Image::CalcWidthBytes(eih.imageSizeX, 16, 4) * eih.imageSizeY;

	if (fileSize - file.GetPosition() < extensionImageTotalBytes)
	{
		::AfxMessageBox(_T("Extension image contents corrupted!"), MB_OK | MB_ICONERROR);
		return false;
	}

	// raw image raad
	horImage.Create(eih.imageSizeX, eih.imageSizeY);
	verImage.Create(eih.imageSizeX, eih.imageSizeY);

	for (long imageIndex = 0; imageIndex < 2; imageIndex++)
	{
		if (imageIndex == 0)
		{
			// 처음에 읽어들인 High Bit Image와 Low Bit Image을 결합하여 첫번째장 이미지로 만든다
			for (long y = 0; y < eih.imageSizeY; y++)
			{
				auto* rawLine = horImage.GetMem(0, y);
				auto* lowLine = firstImage_Low.GetMem(0, y);
				auto* highLine = firstImage_High.GetMem(0, y);

				for (long x = 0; x < eih.imageSizeX; x++)
				{
					rawLine[x] = (highLine[x] << 4) | lowLine[x];
				}
			}
		}
		else
		{
			// 영상 읽어오기
			file.Read(verImage.GetMem(), bmih.biSizeImage * sizeof(uint16_t));
		}
	}

	file.Close();

	return true;
}

bool isNumber(const std::string& str)
{
	for (char const& c : str) {
		if (std::isdigit(c) == 0) return false;
	}
	return true;
}

enum class ImageAcquisitionMode : int32_t  // fov 별 검사 영상 획득 모드
{
  OneShot = 0,
  DualShot,
  FourBucket,
  FourBucket_Dual,
  DOFMeasureShot,
  END,
};

bool MISImageLoader::Save_V3(LPCTSTR pathName, IPVM::Image_16u_C1& first_image, IPVM::Image_16u_C1& second_image, bool is_dual_shot)
{
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	RGBQUAD palette[256];


	// 첫번째 이미지는 상위 비트와 하위 비트를 나눠 저장한다
	IPVM::Image_8u_C1 firstImage_Low;
	IPVM::Image_8u_C1 firstImage_High;
    auto size_x = first_image.GetSizeX();
	auto size_y = first_image.GetSizeY();
    firstImage_Low.Create(size_x, size_y);
    firstImage_High.Create(size_x, size_y);

	if (true)
	{
		auto& firstRaw = first_image;
		for (long y = 0; y < size_y; y++)
		{
			auto* rawLine = firstRaw.GetMem(0, y);
			auto* lowLine = firstImage_Low.GetMem(0, y);
			auto* highLine = firstImage_High.GetMem(0, y);

			for (long x = 0; x < size_x; x++)
			{
#ifdef FILE_EXTENSION_TAG_V3
				lowLine[x] = static_cast<BYTE>(rawLine[x] & 0x0F);
				highLine[x] = static_cast<BYTE>(rawLine[x] >> 4);
#else
				lowLine[x] = ((BYTE*)(rawLine + x))[0];
				highLine[x] = ((BYTE*)(rawLine + x))[1] << 4;
#endif
			}
		}
	}

	const long imageBitmapSize = firstImage_High.GetWidthBytes() * firstImage_High.GetSizeY();
	const long headerSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(palette);

	bmfh.bfType = 0x4d42;
	bmfh.bfSize = headerSize + imageBitmapSize;
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = headerSize;

	bmih.biBitCount = 8;
	bmih.biClrImportant = 0;
	bmih.biClrUsed = 0;
	bmih.biCompression = 0;
	bmih.biHeight = -firstImage_High.GetSizeY();
	bmih.biPlanes = 1;
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biSizeImage = imageBitmapSize;
	bmih.biWidth = firstImage_High.GetSizeX();
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;

	// 2D Images
	for (long idx = 0; idx < 256; idx++)
	{
		palette[idx].rgbBlue = (BYTE)idx;
		palette[idx].rgbGreen = (BYTE)idx;
		palette[idx].rgbRed = (BYTE)idx;
		palette[idx].rgbReserved = (BYTE)0;
	}

	CFile file;

	if (!file.Open(pathName, CFile::modeCreate | CFile::modeWrite))
	{
        ::AfxMessageBox(_T("Cannot open file!"), MB_OK | MB_ICONERROR);
		return false;
	}

	CString title;
	title.Format(_T("Save image file : \"%s\" "), pathName);

	IPVM::AsyncProgress progress(title);

	file.Write(&bmfh, sizeof(bmfh));
	file.Write(&bmih, sizeof(bmih));
	file.Write(&palette, sizeof(palette));
	file.Write(firstImage_High.GetMem(), imageBitmapSize);

	// 추가 정보를 위한 헤더
#ifdef FILE_EXTENSION_TAG_V3
	file.Write(FILE_EXTENSION_TAG_V3, sizeof(FILE_EXTENSION_TAG_V3));
#else
	file.Write(FILE_EXTENSION_TAG_V2, sizeof(FILE_EXTENSION_TAG_V2));
#endif
	file.Write(firstImage_Low.GetMem(), imageBitmapSize);

	ExtensionInfoHeader eih;

	//파일명을 읽고 숫자일 경우에만 fov index로 사용한다.
	CStringA strA(pathName);

	std::string lines = strA.GetBuffer(0);
	int start = strA.ReverseFind('\\') + 1;
	int count = strA.ReverseFind('.') - start;

	auto strAMid = strA.Mid(start, count);

	std::string token = strAMid.GetBuffer();

	if (isNumber(token) && !token.empty())
	{
		eih.fovIndex = stoi(token);
	}
	else
	{
		eih.fovIndex = 0; //number가 없다면 기본 0으로 정한다.
	}

	eih.imageAcquisitionMode = is_dual_shot ? (int32_t)ImageAcquisitionMode::DualShot : (int32_t)ImageAcquisitionMode::OneShot;
	eih.imageSizeX = size_x;
	eih.imageSizeY = size_y;

	file.Write(&eih, sizeof(eih));

	file.Write(second_image.GetMem(), (sizeof(uint16_t) * eih.imageSizeX * eih.imageSizeY));

	file.Close();
	return true;
}

bool MISImageLoader::Save_V4(LPCTSTR pathName, IPVM::Image_16u_C1& first_image, IPVM::Image_16u_C1& second_image, bool is_dual_shot, int nFovNum)
{
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	RGBQUAD palette[256];


	// 첫번째 이미지는 상위 비트와 하위 비트를 나눠 저장한다
	IPVM::Image_8u_C1 firstImage_Low;
	IPVM::Image_8u_C1 firstImage_High;
	auto size_x = first_image.GetSizeX();
	auto size_y = first_image.GetSizeY();
	firstImage_Low.Create(size_x, size_y);
	firstImage_High.Create(size_x, size_y);

	if (true)
	{
		auto& firstRaw = first_image;
		for (long y = 0; y < size_y; y++)
		{
			auto* rawLine = firstRaw.GetMem(0, y);
			auto* lowLine = firstImage_Low.GetMem(0, y);
			auto* highLine = firstImage_High.GetMem(0, y);

			for (long x = 0; x < size_x; x++)
			{
#ifdef FILE_EXTENSION_TAG_V3
				lowLine[x] = static_cast<BYTE>(rawLine[x] & 0x0F);
				highLine[x] = static_cast<BYTE>(rawLine[x] >> 4);
#else
				lowLine[x] = ((BYTE*)(rawLine + x))[0];
				highLine[x] = ((BYTE*)(rawLine + x))[1] << 4;
#endif
			}
		}
	}

	const long imageBitmapSize = firstImage_High.GetWidthBytes() * firstImage_High.GetSizeY();
	const long headerSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(palette);

	bmfh.bfType = 0x4d42;
	bmfh.bfSize = headerSize + imageBitmapSize;
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = headerSize;

	bmih.biBitCount = 8;
	bmih.biClrImportant = 0;
	bmih.biClrUsed = 0;
	bmih.biCompression = 0;
	bmih.biHeight = -firstImage_High.GetSizeY();
	bmih.biPlanes = 1;
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biSizeImage = imageBitmapSize;
	bmih.biWidth = firstImage_High.GetSizeX();
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;

	// 2D Images
	for (long idx = 0; idx < 256; idx++)
	{
		palette[idx].rgbBlue = (BYTE)idx;
		palette[idx].rgbGreen = (BYTE)idx;
		palette[idx].rgbRed = (BYTE)idx;
		palette[idx].rgbReserved = (BYTE)0;
	}

	CFile file;

	if (!file.Open(pathName, CFile::modeCreate | CFile::modeWrite))
	{
		::AfxMessageBox(_T("Cannot open file!"), MB_OK | MB_ICONERROR);
		return false;
	}

	CString title;
	title.Format(_T("Save image file : \"%s\" "), pathName);

	IPVM::AsyncProgress progress(title);

	file.Write(&bmfh, sizeof(bmfh));
	file.Write(&bmih, sizeof(bmih));
	file.Write(&palette, sizeof(palette));
	file.Write(firstImage_High.GetMem(), imageBitmapSize);

	// 추가 정보를 위한 헤더
#ifdef FILE_EXTENSION_TAG_V3
	file.Write(FILE_EXTENSION_TAG_V3, sizeof(FILE_EXTENSION_TAG_V3));
#else
	file.Write(FILE_EXTENSION_TAG_V2, sizeof(FILE_EXTENSION_TAG_V2));
#endif
	file.Write(firstImage_Low.GetMem(), imageBitmapSize);

	ExtensionInfoHeader eih;

	eih.fovIndex = nFovNum;

	eih.imageAcquisitionMode = is_dual_shot ? (int32_t)ImageAcquisitionMode::DualShot : (int32_t)ImageAcquisitionMode::OneShot;
	eih.imageSizeX = size_x;
	eih.imageSizeY = size_y;

	file.Write(&eih, sizeof(eih));

	file.Write(second_image.GetMem(), (sizeof(uint16_t) * eih.imageSizeX * eih.imageSizeY));

	file.Close();
	
	return false;
}
