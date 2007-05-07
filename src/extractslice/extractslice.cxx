

#include "itkCommandLineArgumentParser.h"
#include "CommandLineArgumentHelper.h"
#include "itkImageFileReader.h"
#include "itkExtractImageFilter.h"
#include "itkImageFileWriter.h"
#include <string>
#include <vector>
#include <itksys/SystemTools.hxx>

/** \todo explain rationale.
 *
 *
 */

/** run: A macro to call a function. */
#define run(function,type) \
if ( ComponentType == #type ) \
{ \
  function< type >( inputFileName, outputFileName, slicenumber, which_dimension ); \
}

//-------------------------------------------------------------------------------------

/** Declare ExtractSlice. */
template< class PixelType >
void ExtractSlice(
  const std::string & inputFileName,
  const std::string & outputFileName,
  unsigned int slicenumber,
  unsigned int which_dimension );

/** Declare PrintHelp function. */
void PrintHelp(void);

//-------------------------------------------------------------------------------------

int main( int argc, char ** argv )
{
  if ( argc < 5 )
	{
    PrintHelp();
		return 1;
	}

  /** Create a command line argument parser. */
	itk::CommandLineArgumentParser::Pointer parser = itk::CommandLineArgumentParser::New();
	parser->SetCommandLineArguments( argc, argv );

  /** Get the input file name. */
	std::string inputFileName;
	bool retin = parser->GetCommandLineArgument( "-in", inputFileName );
  if ( !retin )
	{
		std::cerr << "ERROR: You should specify \"-in\"." << std::endl;
		return 1;
	}

  /** Determine input image properties. */
  std::string ComponentType = "short";
  std::string	PixelType; //we don't use this
  unsigned int Dimension = 3;
  unsigned int NumberOfComponents = 1;
  std::vector<unsigned int> imagesize( Dimension, 0 );
  int retgip = GetImageProperties(
    inputFileName,
    PixelType,
    ComponentType,
    Dimension,
    NumberOfComponents,
    imagesize );

  if ( retgip != 0 )
  {
    return 1;
  }
  
  /** Let the user overrule this. */
	bool retpt = parser->GetCommandLineArgument( "-pt", ComponentType );
  
  /** Error checking. */
  if ( NumberOfComponents > 1 )
  { 
    std::cerr << "ERROR: The NumberOfComponents is larger than 1!" << std::endl;
    std::cerr << "Vector images are not supported!" << std::endl;
    return 1;
  }

  /** Get the slicenumber which is to be extracted. */
	unsigned int slicenumber;
	bool retsn = parser->GetCommandLineArgument( "-sn", slicenumber );
  if ( !retsn )
	{
		std::cerr << "ERROR: You should specify \"-sn\"." << std::endl;
		return 1;
	}
  std::string slicenumberstring;
	bool retsns = parser->GetCommandLineArgument( "-sn", slicenumberstring );

  /** Get the dimension in which the slice is to be extracted.
   * The default is the z-direction.
   */
	unsigned int which_dimension = 2;
  bool retd = parser->GetCommandLineArgument( "-d", which_dimension );

  /** Sanity check. */
	if ( slicenumber > imagesize[ which_dimension ] )
	{
		std::cerr << "ERROR: You selected slice number "
			<< slicenumber
      << ", where the input image only has "
			<< imagesize[ which_dimension ]
      << " slices in dimension "
      << which_dimension << "." << std::endl;
		return 1;
	}

  /** Sanity check. */
	if ( which_dimension > Dimension - 1 )
	{
		std::cerr << "ERROR: You selected to extract a slice from dimension "
			<< which_dimension + 1
      << ", where the input image is "
			<< Dimension
      << "D." << std::endl;
		return 1;
	}

  /** Get the outputFileName. */
  std::string direction = "z";
  if ( which_dimension == 0 ) direction = "x";
  else if ( which_dimension == 1 ) direction = "y";
  std::string part1 =
      itksys::SystemTools::GetFilenameWithoutLastExtension( inputFileName );
  std::string part2 =
    itksys::SystemTools::GetFilenameLastExtension( inputFileName );
  std::string	outputFileName = part1 + "_slice_" + direction + "=" + slicenumberstring + part2;
	bool retout = parser->GetCommandLineArgument( "-out", outputFileName );

  /** Run the program. */
  try
	{
    run( ExtractSlice, unsigned char );
    run( ExtractSlice, char );
    run( ExtractSlice, unsigned short );
    run( ExtractSlice, short );
    run( ExtractSlice, float );
  }
	catch( itk::ExceptionObject &e )
	{
		std::cerr << "Caught ITK exception: " << e << std::endl;
		return 1;
	}

	/** Return a value. */
	return 0;

} // end main


//-------------------------------------------------------------------------------------

/** Define ExtractSlice. */
template< class PixelType >
void ExtractSlice(
  const std::string & inputFileName,
  const std::string & outputFileName,
  unsigned int slicenumber,
  unsigned int which_dimension )
{
	/** Some typedef's. */
	typedef itk::Image<PixelType, 3>							Image3DType;
	typedef itk::Image<PixelType, 2>							Image2DType;
	typedef itk::ImageFileReader<Image3DType>			ImageReaderType;
	typedef itk::ExtractImageFilter<
		Image3DType, Image2DType >									ExtractFilterType;
	typedef itk::ImageFileWriter<Image2DType>			ImageWriterType;

	typedef typename Image3DType::RegionType			RegionType;
	typedef typename Image3DType::SizeType				SizeType;
	typedef typename Image3DType::IndexType				IndexType;

	/** Create reader. */
  typename ImageReaderType::Pointer reader = ImageReaderType::New();
  reader->SetFileName( inputFileName.c_str() );
	reader->Update();

	/** Create extractor. */
  typename ExtractFilterType::Pointer extractor = ExtractFilterType::New();
	extractor->SetInput( reader->GetOutput() );

	/** Get the size and set which_dimension to zero. */
  RegionType inputRegion = reader->GetOutput()->GetLargestPossibleRegion();
	SizeType size = inputRegion.GetSize();
  size[ which_dimension ] = 0;

	/** Get the index and set which_dimension to the correct slice. */
	IndexType start = inputRegion.GetIndex();
  start[ which_dimension ] = slicenumber;

	/** Create a desired extraction region and set it into the extractor. */
	RegionType desiredRegion;
  desiredRegion.SetSize(  size  );
  desiredRegion.SetIndex( start );
	extractor->SetExtractionRegion( desiredRegion );

  /** Write the 2D output image. */
  typename ImageWriterType::Pointer writer = ImageWriterType::New();
	writer->SetFileName( outputFileName.c_str() );
	writer->SetInput( extractor->GetOutput() );
	writer->Update();

} // end ExtractSlice

//-------------------------------------------------------------------------------------

/** Define PrintHelp. */
void PrintHelp(void)
{
  std::cout << "pxextractslice extracts a 2D slice from a 3D image." << std::endl;
  std::cout << "Usage:  \npxextractslice" << std::endl;
  std::cout << "  -in      input image filename" << std::endl;
  std::cout << "  [-out]   output image filename" << std::endl;
  std::cout << "  [-pt]    pixel type of input and output images;" << std::endl;
  std::cout << "           default: automatically determined from the first input image." << std::endl;
  std::cout << "  -sn      slice number" << std::endl;
  std::cout << "  [-d]     the dimension from which a slice is extracted, default the z dimension" << std::endl;
  std::cout << "Supported pixel types: (unsigned) char, (unsigned) short, float.\n" << std::endl; 

} // end PrintHelp


