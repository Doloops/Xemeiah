#ifndef __XEM_KERN_ENCODING_H
#define __XEM_KERN_ENCODING_H

namespace Xem
{
  /**
   * Encoding types
   */
  enum Encoding
  {
    Encoding_Unknown,
    Encoding_UTF8,
    Encoding_ISO_8859_1,
    Encoding_US_ASCII,
  };

  class String;

  /**
   * Parse encoding
   */
  Encoding ParseEncoding ( const String encodingName );
};

#endif // __XEM_KERN_ENCODING_H
