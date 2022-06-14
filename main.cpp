#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <cstdio>



static constexpr int imageWidth = 640;
static constexpr int imageHeight = 512;

static unsigned short fileBuf[imageWidth * imageHeight];
static unsigned int intensityBuf[256 * 256];
static unsigned char imageBuf[imageWidth * imageHeight * 3];



class File
{
public:
	File(const char* filename, const char* mode)
	{
#pragma warning(suppress : 4996)
		nativeHandler = std::fopen( filename, mode );
		if (nativeHandler == nullptr)
			throw std::runtime_error( "Can't open the file." );
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
	static constexpr size_t intenElemCount = sizeof( intensityBuf ) / sizeof( intensityBuf[0] );

	std::memset( intensityBuf, 0, sizeof( intensityBuf ) );
	for (int i = 0; i < bufElemCount; i++)
	{
		intensityBuf[buffer[i]]++;
	}
	size_t count;
	for (iBot = 0, count = 0; iBot < intenElemCount && (count += intensityBuf[iBot]) < bufElemCount / 100; iBot++);
	for (iTop = intenElemCount - 1, count = 0; iTop >= 0 && (count += intensityBuf[iTop]) < bufElemCount / 100; iTop--);

#ifndef NDEBUG
	if (windowName != nullptr)
	{
		cv::Mat image = cv::Mat( 400, iTop - iBot + 1, CV_8UC1 );
		for (int i = iBot; i < iTop + 1; i++)
		{
			cv::line( image,
				cv::Point( i - iBot, 0 ),
				cv::Point( i - iBot, (int)std::round( (float)(intensityBuf[i]) / bufElemCount * 1600 ) ),
				cv::Scalar( 1 ) );
		}
		cv::imshow( windowName, image );
	}
#endif // !NDEBUG
}

static void FromFileBufToImageBuf() noexcept
{
	int iBot = 0;
	int iTop = 0;
	CollectIntensity( fileBuf, "Intensity raw (simple)", iBot, iTop );
	unsigned char* imageBufPtr = imageBuf;
	for (const auto& input : fileBuf)
	{
		float v = (float)(input - iBot) / (iTop - iBot);
		v = std::max( 0.0f, std::min( v, 1.0f ) );
		unsigned char output = (unsigned char)std::round( std::pow( v, 0.7f ) * 255.0f );
		*imageBufPtr++ = output;
		*imageBufPtr++ = output;
		*imageBufPtr++ = output;
	}
#ifndef NDEBUG
	CollectIntensity( imageBuf, "Intensity corr (simple)", iBot, iTop );
#endif // !NDEBUG
}

int main()
{
	try
	{
		File f( "dump_13122019_145433.bin", "rb" );
		if (std::fseek( f, 32, SEEK_SET ) != 0)
			throw std::runtime_error( "Some error occurred during skipping the file header." );

		cv::VideoWriter video( "output.mp4", cv::VideoWriter::fourcc( 'X', '2', '6', '4' ), 30.0, cv::Size( imageWidth, imageHeight ) );

		constexpr size_t fileBufElemSize = sizeof( fileBuf[0] );
		constexpr size_t fileBufElemCount = sizeof( fileBuf ) / sizeof( fileBuf[0] );

		size_t bytesReaded;
		while ((bytesReaded = std::fread( fileBuf, fileBufElemSize, fileBufElemCount, f )) == fileBufElemCount)
		{
			FromFileBufToImageBuf();

			cv::Mat image = cv::Mat( imageHeight, imageWidth, CV_8UC3, imageBuf );
			video.write( image );
			cv::imshow( "Display (simple)", image );

			cv::waitKey( 1 );
		}
		if (bytesReaded != 0)
			throw std::runtime_error( "An unexpected end of the dump file." );
	}
	catch (std::exception ex)
	{
		std::cerr << ex.what();
	}
	return 0;
}
