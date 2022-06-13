#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <cstdio>
#include <limits>


static constexpr int width = 640;
static constexpr int height = 512;
static unsigned short fileBuf[width * height];
static unsigned int intenBuf[256 * 256];
static float functionBuf[256 * 256];

static unsigned char imgBuf[width * height];



template<typename ImageBuffer, typename HistogramBuffer>
void IntensityHistogram( ImageBuffer& imageBuffer, HistogramBuffer& histogramBuffer, const char* windowName )
{
	constexpr size_t imgElemCount = sizeof( imageBuffer ) / sizeof( imageBuffer[0] );
	constexpr size_t histElemCount = sizeof( histogramBuffer ) / sizeof( histogramBuffer[0] );
	static_assert(histElemCount >= std::numeric_limits<std::remove_reference_t<decltype(imageBuffer[0])>>::max(), "Histogram buffer is too small.");
	static_assert(imgElemCount <= std::numeric_limits< std::remove_reference_t<decltype(histogramBuffer[0])>>::max(), "Histogram element is too small.");

	std::memset( histogramBuffer, 0, sizeof( histogramBuffer ) );
	for (int i = 0; i < imgElemCount; i++)
	{
		histogramBuffer[imageBuffer[i]]++;
	}

	if (windowName != nullptr)
	{
		size_t count;
		int iBot, iTop;
		for (iBot = 0, count = 0; iBot < histElemCount && (count += histogramBuffer[iBot]) < histElemCount / 20; iBot++);
		for (iTop = histElemCount - 1, count = 0; iTop >= 0 && (count += histogramBuffer[iTop]) < histElemCount / 20; iTop--);

		cv::Mat image = cv::Mat( 400, iTop - iBot + 1, CV_8UC1 );
		for (int i = iBot; i < iTop + 1; i++)
		{
			cv::line( image,
				cv::Point( i - iBot, 0 ),
				cv::Point( i - iBot, (int)std::round( (float)(histogramBuffer[i]) / imgElemCount * 1600 ) ),
				cv::Scalar( 1 ) );
		}
		cv::imshow( windowName, image );
	}
}

template<size_t ImgElemCount, typename FunctionBuffer, typename HistogramBuffer>
void FunctionHistogram( FunctionBuffer& functionBuffer, HistogramBuffer& histogramBuffer, const char* windowName )
{
	constexpr size_t histElemCount = sizeof( histogramBuffer ) / sizeof( histogramBuffer[0] );
	constexpr size_t funcElemCount = sizeof( functionBuffer ) / sizeof( functionBuffer[0] );
	static_assert(funcElemCount >= histElemCount, "Function buffer element count is too small.");

	size_t count = 0;
	for (size_t i = 0; i < histElemCount; i++)
	{
		count += histogramBuffer[i];
		functionBuffer[i] = (float)count / ImgElemCount;
	}

	if (windowName != nullptr)
	{
		int iBot, iTop;
		for (iBot = 0; iBot < funcElemCount && functionBuffer[iBot] < 0.01f; iBot++);
		for (iTop = funcElemCount - 1; iTop >= 0 && functionBuffer[iTop] > 0.99f; iTop--);

		cv::Mat image = cv::Mat( 100, iTop - iBot + 1, CV_8UC1 );
		for (int i = iBot; i < iTop + 1; i++)
		{
			cv::line( image,
				cv::Point( i - iBot, 0 ),
				cv::Point( i - iBot, (int)std::round( functionBuffer[i] * 100 ) ),
				cv::Scalar( 1 ) );
		}
		cv::imshow( windowName, image );
	}
}



class File
{
public:
	File(const char* filename, const char* mode)
	{
#pragma warning(suppress : 4996)
		nativeHandler = std::fopen( filename, mode );
		if (nativeHandler == nullptr)
			throw std::runtime_error( "Can't open the dump file." );
	}

	operator std::FILE* () const noexcept
	{
		return nativeHandler;
	}

	~File()
	{
		if (nativeHandler != nullptr)
			std::fclose( nativeHandler );
	}

private:
	std::FILE* nativeHandler = nullptr;
};

template<typename Buffer>
void CollectIntensity( Buffer& buffer, const char* windowName, int& iBot, int& iTop )
{
	static constexpr size_t bufElemCount = sizeof( buffer ) / sizeof( buffer[0] );
	static constexpr size_t intenElemCount = sizeof( intenBuf ) / sizeof( intenBuf[0] );

	std::memset( intenBuf, 0, sizeof( intenBuf ) );
	for (int i = 0; i < bufElemCount; i++)
	{
		intenBuf[buffer[i]]++;
	}
	size_t count;
	for (iBot = 0, count = 0; iBot < intenElemCount && (count += intenBuf[iBot]) < bufElemCount / 50; iBot++);
	for (iTop = intenElemCount - 1, count = 0; iTop >= 0 && (count += intenBuf[iTop]) < bufElemCount / 50; iTop--);

	cv::Mat image = cv::Mat( 400, iTop - iBot + 1, CV_8UC1 );
	for (int i = iBot; i < iTop + 1; i++)
	{
		cv::line( image,
			cv::Point( i - iBot, 0 ),
			cv::Point( i - iBot, (int)std::round( (float)(intenBuf[i]) / bufElemCount * 1600 ) ),
			cv::Scalar( 1 ) );
	}
	cv::imshow( windowName, image );
}

void ShowIntensity()
{

}

template<typename Buffer>
void ColorCorrection( Buffer& buffer, const char* windowName )
{
	
}

int main()
{
	try
	{
		File f( "dump_13122019_145433.bin", "rb" );
		if (std::fseek( f, 32, SEEK_SET ) != 0)
			throw std::runtime_error( "Some error occurred during skipping the file header." );

		cv::VideoWriter video( "out.avi", cv::VideoWriter::fourcc( 'm', 'p', '4', 'v' ), 30.0, cv::Size( width, height ) );

		size_t count;
		int iBot = 0;
		int iTop = 0;
		while ((count = std::fread( fileBuf, sizeof( fileBuf[0] ), sizeof( fileBuf ) / sizeof( fileBuf[0] ), f )) ==
			sizeof( fileBuf ) / sizeof( fileBuf[0] ))
		{
			//IntensityHistogram( fileBuf, intenBuf, "Intensity raw" );
			//FunctionHistogram<width* height>( functionBuf, intenBuf, "Function" );
			CollectIntensity( fileBuf, "Intensity raw (simple)", iBot, iTop );
			for (int i = 0; i < sizeof( fileBuf ) / sizeof( fileBuf[0] ); i++)
			{
				float v = (float)(fileBuf[i] - iBot) / (iTop - iBot);
				v = std::max( 0.0f, std::min( v, 1.0f ) );
				//imgBuf[i] = (unsigned char)std::round( std::pow(v, 1.4f) * 255.0f );
				imgBuf[i] = (unsigned char)std::round( v * 255.0f );
				///imgBuf[i] = (unsigned char)std::round( std::pow( functionBuf[fileBuf[i]], 0.8f ) * 255 );
			}
			CollectIntensity( imgBuf, "Intensity corr (simple)", iBot, iTop );
			//IntensityHistogram( imgBuf, intenBuf, "Intensity corr" );

			cv::Mat image = cv::Mat( height, width, CV_8UC1, imgBuf );
			video.write( image );
			cv::imshow( "Display (simple)", image );

			IntensityHistogram( fileBuf, intenBuf, "Intensity raw (complex)" );
			FunctionHistogram<width* height>( functionBuf, intenBuf, "Function (complex)" );
			for (int i = 0; i < sizeof( fileBuf ) / sizeof( fileBuf[0] ); i++)
			{
				///float v = functionBuf[fileBuf[i]];
				///float x = (v - 0.5f) * 4.0f - 1.0f;
				///float shift = x * x / -4.0f;
				///x += (v > 0.5f) ? -shift : shift;
				imgBuf[i] = (unsigned char)std::round( std::pow( functionBuf[fileBuf[i]], 0.8f ) * 255 );
				///imgBuf[i] = (unsigned char)std::round( x * 255 );
			}
			IntensityHistogram( imgBuf, intenBuf, "Intensity corr (complex)" );

			cv::Mat image2 = cv::Mat( height, width, CV_8UC1, imgBuf );
			video.write( image2 );
			cv::imshow( "Display (complex)", image2 );

			cv::waitKey( 0 );
		}
		if (count != 0)
			throw std::runtime_error( "An unexpected end of dump file." );
	}
	catch (std::exception ex)
	{
		std::cerr << ex.what();
	}
	return 0;
}
