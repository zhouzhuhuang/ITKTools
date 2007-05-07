#include "itkCommandLineArgumentParser.h"
#include "CommandLineArgumentHelper.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include <itksys/SystemTools.hxx>

#include "itkBinaryBallStructuringElement.h"
#include "itkGrayscaleErodeImageFilter.h"

//-------------------------------------------------------------------------------------

/** run: A macro to call a function. */
#define run(function,type,dim) \
if ( ComponentType == #type && Dimension == dim ) \
{ \
    typedef itk::Image< type, dim > InputImageType; \
    function< InputImageType >( inputFileName, outputFileName, Radius, boundaryCondition ); \
}

//-------------------------------------------------------------------------------------

/** Declare ErodeImage. */
template< class InputImageType >
void ErodeImage( const std::string & inputFileName, const std::string & outputFileName,
                std::vector<unsigned int> radius, const std::string & boundaryCondition );

/** Declare PrintHelp. */
void PrintHelp(void);

//-------------------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
	/** Check arguments for help. */
	if ( argc < 5 || argc > 13 )
	{
		PrintHelp();
		return 1;
	}

	/** Create a command line argument parser. */
	itk::CommandLineArgumentParser::Pointer parser = itk::CommandLineArgumentParser::New();
	parser->SetCommandLineArguments( argc, argv );

	/** Get arguments. */
	std::string	inputFileName = "";
	bool retin = parser->GetCommandLineArgument( "-in", inputFileName );

	std::string	outputFileName = 
    itksys::SystemTools::GetFilenameWithoutLastExtension(inputFileName);
  std::string ext =
    itksys::SystemTools::GetFilenameLastExtension(inputFileName);
	outputFileName += "ERODED" + ext;
	bool retout = parser->GetCommandLineArgument( "-out", outputFileName );

  std::string boundaryCondition = "";
	bool retbc = parser->GetCommandLineArgument( "-bc", boundaryCondition );

	std::vector<unsigned int> radius;
	bool retr = parser->GetCommandLineArgument( "-r", radius );

  /** Check if the required arguments are given. */
	if ( !retin )
	{
		std::cerr << "ERROR: You should specify \"-in\"." << std::endl;
		return 1;
	}
	if ( !retr )
	{
		std::cerr << "ERROR: You should specify \"-r\"." << std::endl;
		return 1;
	}
  
  /** Determine image properties. */
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
  if ( retgip !=0 )
  {
    return 1;
  }
  std::cout << "The input image has the following properties:" << std::endl;
  /** Do not bother the user with the difference between pixeltype and componenttype:*/
  //std::cout << "\tPixelType:          " << PixelType << std::endl;
  std::cout << "\tPixelType:          " << ComponentType << std::endl;
  std::cout << "\tDimension:          " << Dimension << std::endl;
  std::cout << "\tNumberOfComponents: " << NumberOfComponents << std::endl;
  
  /** Let the user overrule this */
	bool retdim = parser->GetCommandLineArgument( "-dim", Dimension );
	bool retpt = parser->GetCommandLineArgument( "-pt", ComponentType );
  if (retdim | retpt)
  {
    std::cout << "The user has overruled this by specifying -pt and/or -dim:" << std::endl;
    std::cout << "\tPixelType:          " << ComponentType << std::endl;
    std::cout << "\tDimension:          " << Dimension << std::endl;
    std::cout << "\tNumberOfComponents: " << NumberOfComponents << std::endl;
  }

  if (NumberOfComponents > 1)
  { 
    std::cerr << "ERROR: The NumberOfComponents is larger than 1!" << std::endl;
    std::cerr << "Vector images are not supported!" << std::endl;
    return 1; 
  }
  
  /** Get rid of the possible "_" in ComponentType. */
	ReplaceUnderscoreWithSpace( ComponentType );

  /** Check radius. */
	if ( retr )
	{
		if( radius.size() != Dimension && radius.size() != 1 )
		{
			std::cout << "ERROR: The number of radii should be 1 or Dimension." << std::endl;
			return 1;
		}
	}

	/** Get the radius. */
	std::vector<unsigned int> Radius( Dimension, radius[ 0 ] );
	if ( retr && radius.size() == Dimension )
	{
		for ( unsigned int i = 1; i < Dimension; i++ )
		{
			Radius[ i ] = radius[ i ];
      if ( Radius[ i ] < 1 )
		  {
		    std::cout << "ERROR: No nonpositive numbers are allowed in radius." << std::endl;
		 	  return 1;
		  }
		}
	}
  
	/** Run the program. */
	try
	{
		run(ErodeImage,unsigned char,2);
		run(ErodeImage,unsigned char,3);
		run(ErodeImage,char,2);
		run(ErodeImage,char,3);
		run(ErodeImage,unsigned short,2);
		run(ErodeImage,unsigned short,3);
		run(ErodeImage,short,2);
		run(ErodeImage,short,3);
	}
	catch( itk::ExceptionObject &e )
	{
		std::cerr << "Caught ITK exception: " << e << std::endl;
		return 1;
	}
	
	/** End program. */
	return 0;

} // end main


	/**
	 * ******************* ErodeImage *******************
	 */

template< class InputImageType >
void ErodeImage( 
  const std::string & inputFileName,
  const std::string & outputFileName,
	std::vector<unsigned int> radius,
  const std::string & boundaryCondition )
{
	/** Typedefs. */
	typedef itk::ImageFileReader< InputImageType >			ReaderType;
	typedef itk::ImageFileWriter< InputImageType >			WriterType;

	typedef typename InputImageType::PixelType					PixelType;
	const unsigned int Dimension = InputImageType::ImageDimension;
	
	typedef itk::BinaryBallStructuringElement<
		PixelType, Dimension >														StructuringElementType;
	typedef typename StructuringElementType::RadiusType					RadiusType;
	typedef itk::GrayscaleErodeImageFilter<
		InputImageType, InputImageType,
		StructuringElementType >													ErodeFilterType;
  typedef typename  
    ErodeFilterType::DefaultBoundaryConditionType     BoundaryConditionType;

	/** DECLARATION'S. */
	typename ReaderType::Pointer reader = ReaderType::New();
	typename WriterType::Pointer writer = WriterType::New();
	typename ErodeFilterType::Pointer erosion = ErodeFilterType::New();
	RadiusType	radiusarray;
	StructuringElementType	S_ball;

	/** Setup the reader. */
	reader->SetFileName( inputFileName.c_str() );

	/** Create and fill the radius. */
	radiusarray.Fill( 1 );
	for ( unsigned int i = 0; i < Dimension; i++ )
	{
		radiusarray.SetElement( i, radius[ i ] );
	}

	/** Create the structuring element and set it into the erosion filter. */
	S_ball.SetRadius( radiusarray );
	S_ball.CreateStructuringElement();
	erosion->SetKernel( S_ball );

  /** Set a boundary condition value. This is the value outside the image.
   * By default it is set to max(PixelType). */
  BoundaryConditionType bc;
  PixelType bcValue = itk::NumericTraits<PixelType>::max();
  if ( boundaryCondition != "")
  {
    if ( itk::NumericTraits<PixelType>::is_integer )
    {
      bcValue = static_cast<PixelType>( atoi( boundaryCondition.c_str() ) );
    }
    else
    {
      bcValue = static_cast<PixelType>( atof( boundaryCondition.c_str() ) );
    }
    bc.SetConstant( bcValue );
    erosion->OverrideBoundaryCondition(&bc);
  }

	/** Connect the pipeline. */
	erosion->SetInput( reader->GetOutput() );

	/** Write the output image. */
	writer->SetFileName( outputFileName.c_str() );
	writer->SetInput( erosion->GetOutput() );
	writer->Update();

} // end ErodeImage


	/**
	 * ******************* PrintHelp *******************
	 */
void PrintHelp()
{
	std::cout << "Usage:" << std::endl << "pxerodeimage" << std::endl;
	std::cout << "  -in      inputFilename" << std::endl;
	std::cout << "  [-out]   outputFilename, default in + ERODED + extension(inputFilename)" << std::endl;
	std::cout << "  -r       radius" << std::endl;
  std::cout << "  [-dim]   dimension, default: automatically determined from image" << std::endl;
  std::cout << "  [-pt]    pixelType, default: automatically determined from image" << std::endl;
  std::cout << "  [-bc]    boundaryCondition; the grey value outside the image; default: max(PixelType)" << std::endl;
	std::cout << "Supported: 2D, 3D, (unsigned) short, (unsigned) char." << std::endl;
} // end PrintHelp

