#include <Magick++.h>

#include <Xemeiah/xem-imagemagick/xem-imagemagickmodule.h>

#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/dom/blobref.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XemIM Log // Debug

namespace Xem
{
  __XProcessorLib_DECLARE_LIB ( XemImageMagick, "xem-imagemagick" );
  __XProcessorLib_REGISTER_MODULE ( XemImageMagick, XemImageMagickModuleForge );

#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/xem-imagemagick/builtin-keys/xem_imagemagick>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  XemImageMagickModuleForge::XemImageMagickModuleForge(Store& store) :
    XProcessorModuleForge(store), xem_im(store.getKeyCache())
  {

  }

  XemImageMagickModuleForge::~XemImageMagickModuleForge()
  {

  }

  void
  XemImageMagickModuleForge::instanciateModule(XProcessor& xprocessor)
  {
    XProcessorModule* module = new XemImageMagickModule(xprocessor, *this);
    xprocessor.registerModule(module);
  }

  XemImageMagickModule::XemImageMagickModule(XProcessor& xproc, XemImageMagickModuleForge& moduleForge) :
    XProcessorModule(xproc, moduleForge), xem_im(moduleForge.xem_im)
  {

  }

  XemImageMagickModuleForge& XemImageMagickModule::getXemImageMagickModuleForge()
  {
    return dynamic_cast<XemImageMagickModuleForge&> ( moduleForge );
  }

  void
  XemImageMagickModule::install()
  {

  }

  void
  XemImageMagickModuleForge::install()
  {
    // registerFunction ( xem_fs.get_current_time(), &XemImageMagickModule::functionGetCurrentTime );
    registerHandler ( xem_im.crop(), &XemImageMagickModule::instructionCrop );
    registerHandler ( xem_im.scale(), &XemImageMagickModule::instructionScale );
    registerHandler ( xem_im.reflection(), &XemImageMagickModule::instructionReflection );
    registerHandler ( xem_im.picture_mess(), &XemImageMagickModule::instructionPictureMess );
  }


  void
  XemImageMagickModuleForge::registerEvents(Document& doc)
  {

  }

  Magick::Image getMagickImage ( Xem::BlobRef source )
  {
    __ui64 sz = source.getSize();
    char* tmp = (char*) malloc ( sz );
    __ui64 remains = sz;
    __ui64 offset = 0;
    while ( remains )
      {
        void* window;
        __ui64 windowSize = source.getPiece(&window, SegmentSizeMax, offset );
        AssertBug ( windowSize, "Could not fetch !\n" );
        if ( windowSize > remains )
          windowSize = remains;
        memcpy ( &(tmp[offset]), window, windowSize );
        remains -= windowSize;
        offset += windowSize;
      }
    return Magick::Image(Magick::Blob(tmp, sz));
  }

  void setMagickImage ( Magick::Image src, Xem::BlobRef target )
  {
    Magick::Blob blob;
    src.write(&blob);

    Log ( "Blob length : %lu\n", (unsigned long) blob.length() );
    target.writePiece(blob.data(), blob.length(), 0);

    Log ( "Post save blob length %llu\n", target.getSize() );
  }

  void XemImageMagickModule::instructionCrop ( __XProcHandlerArgs__ )
  {
    BlobRef source = XPath (getXProcessor(),item,xem_im.source()).evalAttribute();
    BlobRef target = XPath (getXProcessor(),item,xem_im.target()).evalAttribute();
    Integer width = item.getEvaledAttr(getXProcessor(),xem_im.width()).toInteger();
    Integer height = item.getEvaledAttr(getXProcessor(),xem_im.height()).toInteger();

    Magick::Image image = getMagickImage ( source );

    Log ( "Crop to : %lldx%lld\n", width, height );

    image.crop( Magick::Geometry(width, height, 0, 0 ) );

    Log ( "Got image %ux%u\n", image.columns(), image.rows() );

    target.getDocument().grantWrite(getXProcessor());
    setMagickImage ( image, target );
  }

  void XemImageMagickModule::instructionScale ( __XProcHandlerArgs__ )
  {
    BlobRef source = XPath (getXProcessor(),item,xem_im.source()).evalAttribute();
    BlobRef target = XPath (getXProcessor(),item,xem_im.target()).evalAttribute();
    Integer width = item.getEvaledAttr(getXProcessor(),xem_im.width()).toInteger();
    Integer height = item.getEvaledAttr(getXProcessor(),xem_im.height()).toInteger();

    Magick::Image image = getMagickImage ( source );
    image.scale( Magick::Geometry(width, height, 0, 0 ) );

    Log ( "Got image %ux%u\n", image.columns(), image.rows() );

    target.getDocument().grantWrite(getXProcessor());
    setMagickImage ( image, target );
  }

  void XemImageMagickModule::instructionReflection ( __XProcHandlerArgs__ )
  {
    BlobRef source = XPath (getXProcessor(),item,xem_im.source()).evalAttribute();
    BlobRef target = XPath (getXProcessor(),item,xem_im.target()).evalAttribute();
    Number ratio = item.getEvaledAttr(getXProcessor(),xem_im.ratio()).toNumber();

    Log ( "Ratio : %g\n", ratio );

    Magick::Image image = getMagickImage ( source );
    Magick::Image reflection = image;

    reflection.flip();
    reflection.crop(Magick::Geometry(reflection.columns(), reflection.rows() * ratio, 0, 0 ));

    reflection.matte(true);

    unsigned int gradientYOffset = reflection.rows();
    Magick::Image gradient(Magick::Geometry(reflection.columns(), reflection.rows() + gradientYOffset ),
        Magick::Color(0,0,0,QuantumRange) );
    gradient.read("gradient:black-transparent");
    gradient.crop(Magick::Geometry(reflection.columns(), reflection.rows(), 0, gradientYOffset ));
    reflection.composite(gradient,Magick::Geometry(0,0),Magick::CopyOpacityCompositeOp);

    target.getDocument().grantWrite(getXProcessor());
    target.setMimeType("image/png");
    Log ( "Format : %s\n", reflection.magick().c_str() );
    reflection.magick("PNG");

    setMagickImage ( reflection, target );
  }

  void XemImageMagickModule::instructionPictureMess ( __XProcHandlerArgs__ )
  {
    srand(time(NULL));
    NodeSet sourceBlobs;
    XPath (getXProcessor(),item,xem_im.source()).eval(sourceBlobs);
    BlobRef target = XPath (getXProcessor(),item,xem_im.target()).evalAttribute();

    Integer width = item.getEvaledAttr(getXProcessor(),xem_im.width()).toInteger();
    Integer height = item.getEvaledAttr(getXProcessor(),xem_im.height()).toInteger();

    std::list<Magick::Image> sources;
    Magick::Geometry geometry(0,0);

    for ( NodeSet::iterator iter(sourceBlobs) ; iter ; iter++ )
      {
        BlobRef blob = iter->toAttribute();
        Magick::Image image = getMagickImage(blob);
        sources.push_back(image);
        if ( geometry.width() < image.columns() )
          geometry.width(image.columns());
        if ( geometry.height() < image.rows() )
          geometry.height(image.rows());
      }
    Log ( "Loaded %lu images, max geometry is %ux%u\n", (unsigned long) sources.size(), geometry.width(), geometry.height());

    Magick::Geometry messGeometry (width, height);
    messGeometry.aspect(true);
    double opRatio = 0.6f;
    double maxAngle = 30.0f;
    Magick::Geometry opGeometry (messGeometry.width()*opRatio,messGeometry.height()*opRatio);
    Magick::Image mess(messGeometry,Magick::Color(0,0,0,QuantumRange));
    mess.matte(true);

    Log ( "messGeometry : %ux%u\n", messGeometry.width(), messGeometry.height() );
    Log ( "opGeometry : %ux%u\n", opGeometry.width(), opGeometry.height() );

    for ( std::list<Magick::Image>::iterator iter = sources.begin() ; iter != sources.end() ; iter++ )
      {
        Magick::Image& image = *iter;
        image.scale(opGeometry);
        image.matte(true);

        image.backgroundColor(Magick::Color(0,0,0,QuantumRange));
        image.matteColor(Magick::Color(0,0,0,QuantumRange));
        image.fillColor(Magick::Color(0,0,0,QuantumRange));
        image.borderColor(Magick::Color(0,0,0,QuantumRange));

        double angle = ((double) rand()) / RAND_MAX;
        angle *= maxAngle*2;
        angle -= maxAngle;

	Log ( "Angle : %g\n", angle );
        Log ( "Pre-rotate : %ux%u\n", image.columns(), image.rows() );

        image.rotate(angle);

        Log ( "Post-rotate : %ux%u\n", image.columns(), image.rows() );

        Magick::Image canvas(messGeometry,Magick::Color(0,0,0,QuantumRange));
        
        Log ( "Canvas-sizesize : %ux%u\n", canvas.columns(), canvas.rows() );

	canvas.composite(image,image.size(),Magick::OverCompositeOp);

        int offx = rand() % (messGeometry.width() - image.columns());
        int offy = rand() % (messGeometry.height() - image.rows());
        Log ( "Roll : %ux%u\n", offx, offy );

        canvas.roll(Magick::Geometry(0,0,offx,offy));
        mess.composite(canvas,messGeometry,Magick::OverCompositeOp);
      }

    mess.magick("PNG");
    setMagickImage ( mess, target );
  }
};
