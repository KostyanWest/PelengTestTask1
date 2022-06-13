#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <cstdio>


static constexpr int width = 640;
static constexpr int height = 512;
static unsigned short fileBuf[width * height];
static unsigned short intenBuf[256 * 256];

static unsigned char imgBuf[width * height];




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

int main()
{
	try
	{
		File f( "dump_13122019_145433.bin", "rb" );
		if (std::fseek( f, 32, SEEK_SET ) != 0)
			throw std::runtime_error( "Some error occurred during skipping the file header." );

		cv::VideoWriter video( "out.avi", cv::VideoWriter::fourcc( 'M', 'J', 'P', 'G' ), 60.0, cv::Size( width, height ) );

		size_t count;
		int iMin = 0;
		int iMax = 0;
		while ((count = std::fread( fileBuf, sizeof( fileBuf[0] ), sizeof( fileBuf ) / sizeof( fileBuf[0] ), f )) ==
			sizeof( fileBuf ) / sizeof( fileBuf[0] ))
		{
			std::memset( intenBuf, 0, sizeof( intenBuf ) );
			for (int i = 0; i < sizeof( fileBuf ) / sizeof( fileBuf[0] ); i++)
			{
				intenBuf[fileBuf[i]]++;
			}

			if (iMin == 0)
				for (iMin = 0; iMin < sizeof( intenBuf ) / sizeof( intenBuf[0] ) && intenBuf[iMin] == 0; iMin++);
			if (iMax == 0)
				for (iMax = sizeof( intenBuf ) / sizeof( intenBuf[0] ) - 1; iMax >= 0 && intenBuf[iMax] == 0; iMax--);

			for (int i = 0; i < sizeof( fileBuf ) / sizeof( fileBuf[0] ); i++)
			{
				float v = (float)(fileBuf[i] - iMin) / (iMax - iMin);
				imgBuf[i] = std::round( std::pow(v, 1.5f) * 255.0f );
				
			}
			cv::Mat image = cv::Mat( height, width, CV_8UC1, imgBuf );
			video.write( image );
			cv::imshow( "Display", image );
			cv::waitKey( 0 );
		}
		if (count != 0)
			throw std::runtime_error( "An unexpected end of dump file." );
	}
	catch (std::exception ex)
	{
		std::cerr << ex.what();
	}

	cv::Mat image = cv::Mat( height, width, CV_8UC1, fileBuf, 512*2 );
	cv::imshow( "Display Window", image );
	cv::waitKey( 0 );
	return 0;
}
