/*
 * querydocument-parse.cpp
 *
 *  Created on: 6 févr. 2010
 *      Author: francois
 */

#include <Xemeiah/webserver/querydocument.h>
#include <Xemeiah/kern/store.h>

#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/dom/blobref.h>

#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/webserver/webserver.h>
#include <Xemeiah/webserver/webservermodule.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_QParse Debug

namespace Xem
{
  enum QParseState
  {
    QParseState_HeadLine,
    QParseState_URLStart,
    QParseState_URL,
    QParseState_URLParam,
    QParseState_URLParamValue,
    QParseState_Protocol,
    QParseState_FieldName,
    QParseState_PostFieldName, // after the ':' field name symbol, expecting a space ' '
    QParseState_Value,
    QParseState_ResponseCode,
    QParseState_ResponseString,
    QParseState_BeginLine,
    QParseState_CookieStart,
    QParseState_CookieName,
    QParseState_CookieValue,
    QParseState_FinishedHTTPHeader
  };

  void QueryDocument::parseQuery ( XProcessor& xproc, bool isQuery )
  {
    ElementRef cookiesList = queryElement.getDocument().createElement ( queryElement, xem_web.cookies() );
    queryElement.appendLastChild ( cookiesList );

    ElementRef headersList = queryElement.getDocument().createElement ( queryElement, xem_web.headers() );
    queryElement.appendLastChild ( headersList );

    QParseState state = QParseState_HeadLine;
    String accu = "";
    String method;
    ElementRef headerElement(*this);
    ElementRef responseElement(*this);
    ElementRef urlElement(*this);
    ElementRef urlParams(*this);
    ElementRef urlParam(*this);
    ElementRef cookieElement(*this);

    __ui64 contentLength = 0;
    bool transferEncodingChunked = false;

#define Invalid() throwException(Exception, "Invalid character %d at state %d\n", r, state );

    while ( state != QParseState_FinishedHTTPHeader && !reader.isFinished())
      {
        int r = reader.getNextChar();
        if ( r >= 0x80 )
          {
            throwException ( Exception, "Invalid char %d\n", r );
          }
        // Log_QParse ( "state=%d, r=%x (%d, %c)\n", state, r, r, r );
        switch ( state )
        {
        case QParseState_HeadLine:
          if ( r == ' ' )
            {
              Log_QParse ( "Method : '%s'\n", accu.c_str() );
              if ( isQuery )
                {
                  if ( accu == "GET" || accu == "POST" )
                    {
                      ElementRef methodElement = addTextualElement ( queryElement, xem_web.method(), accu );
                      method = methodElement.getText();
                      state = QParseState_URLStart;
                      accu = "";
                      continue;
                    }
                  else
                    Invalid();
                }
              else
                {
                  if ( accu == "HTTP/1.1" )
                    {
                      Log_QParse ( "Got a Response, protocol=%s\n", accu.c_str() );
                      addTextualElement ( queryElement, xem_web.protocol(), accu );
                      accu = "";
                      state = QParseState_ResponseCode;
                      responseElement = createElement ( headersList, xem_web.response() );
                      headersList.appendLastChild ( responseElement );
                    }
                  else
                    {
                      Invalid();
                    }
                }
            }
          else if ( r == '\r' || r == '\n' )
            {
              Invalid();
            }
          accu.appendUtf8(r);
          break;
        case QParseState_URLStart:
          if ( r == '/' )
            {
              state = QParseState_URL;
              continue;
            }
          Invalid();
          break;
        case QParseState_URL:
          if ( r == ' ' || r == '?' )
            {
              Log_QParse ( "Url : '%s'\n", accu.c_str() );
              urlElement = createElement ( queryElement, xem_web.url() );
              queryElement.appendLastChild ( urlElement );
              addTextualElement ( urlElement, xem_web.base(), accu );

              accu = "";
              if ( r == ' ' )
                {
                  state = QParseState_Protocol;
                }
              else
                {
                  state = QParseState_URLParam;
                }
              continue;
            }
          else if ( r == '?' )
            {
              Invalid();
            }
          else if ( r == '\r' || r == '\n' )
            {
              Invalid();
            }
          else if ( r == '%' )
            {
              Invalid ();
            }
          accu.appendUtf8(r);
          break;
        case QParseState_URLParam:
          if ( r == '\n' || r == '%' || r == '\r' )
            {
              Invalid();
            }
          else if ( r == '=' || r == ' ' || r == '&' )
            {
              if ( ! urlParams )
                {
                  urlParams = createElement ( urlElement, xem_web.parameters() );
                  urlElement.appendLastChild ( urlParams );
                }
              Log_QParse ( "Param : '%s'\n", accu.c_str() );
              urlParam = createElement ( urlParams, xem_web.param() );
              urlParams.appendLastChild ( urlParam );
              urlParam.addAttr ( xem_web.name(), accu );
              accu = "";
              if ( r == '=' )
                state = QParseState_URLParamValue;
              else if ( r == '&' )
                state = QParseState_URLParam;
              else if ( r == ' ' )
                state = QParseState_Protocol;
              else
                { Invalid(); }
              continue;
            }
          accu.appendUtf8(r);
          break;
        case QParseState_URLParamValue:
          if ( r == '&' || r == ' ')
            {
              Log_QParse ( "Param Value : '%s'\n", accu.c_str() );
              ElementRef valueNode = createTextNode ( urlParam, accu );
              urlParam.appendLastChild ( valueNode );
              urlParam = ElementRef(*this);
              accu = "";
              if ( r == '&' )
                state = QParseState_URLParam;
              else
                state = QParseState_Protocol;
              continue;
            }
          accu.appendUtf8(r);
          break;
        case QParseState_Protocol:
          if ( r == ' ' )
            {
              Invalid();
            }
          else if ( r == '\r' )
            {
              continue;
            }
          else if ( r == '\n' )
            {
              Log_QParse ( "Protocol = '%s'\n", accu.c_str() );
              addTextualElement ( queryElement, xem_web.protocol(), accu );
              accu = "";
              state = QParseState_BeginLine;
              continue;
            }
          accu.appendUtf8(r);
          break;
        case QParseState_ResponseCode:
          if ( r == ' ' )
            {
              Log_QParse ( "ResponseCode : %s\n", accu.c_str() );
              responseElement.addAttr ( xem_web.response_code(), accu );
              accu = "";
              state = QParseState_ResponseString;
              continue;
            }
          if ( '0' <= r && r <= '9' )
            {
              accu.appendUtf8(r);
              continue;
            }
          Invalid();
          break;
        case QParseState_ResponseString:
          if ( r == '\r' )
            continue;
          else if ( r == '\n' )
            {
              Log_QParse ( "ResponseString : %s\n", accu.c_str() );
              responseElement.addAttr ( xem_web.response_string(), accu );
              accu = "";
              state = QParseState_BeginLine;
              continue;
            }
          accu.appendUtf8(r);
          break;
        case QParseState_BeginLine:
          if ( r == '\r' )
            continue;
          else if ( r == '\n' )
            {
              state = QParseState_FinishedHTTPHeader;
              break;
            }
          else if ( r == ' ' )
            {
              Invalid();
            }
          accu.appendUtf8(r);
          state = QParseState_FieldName;
          continue;
        case QParseState_FieldName:
          if ( r == ':' )
            {
              Log_QParse ( "Field : '%s'\n", accu.c_str() );
              if ( accu == "Cookie" )
                {
                  accu = "";
                  state = QParseState_CookieStart;
                  continue;
                }
              headerElement = createElement ( headersList, xem_web.param() );
              headersList.appendLastChild ( headerElement );
              headerElement.addAttr ( xem_web.name(), accu );
              accu = "";
              state = QParseState_PostFieldName;
              continue;
            }
          else if ( r == ' ' || r == '\n' || r == '\r' )
            {
               Invalid();
            }
          accu.appendUtf8(r);
          break;
        case QParseState_PostFieldName:
          if ( r == ' ' )
            {
              state = QParseState_Value;
              continue;
            }
          Invalid();
        case QParseState_Value:
          if ( r == '\r')
            continue;
          else if ( r == '\n' )
            {
              Log_QParse ( "Field value : '%s'\n", accu.c_str() );
              ElementRef valueNode = createTextNode ( headerElement, accu );
              headerElement.appendLastChild ( valueNode );
              String fieldName = headerElement.getAttr(xem_web.name());
#ifdef __XEM_WEBSERVER_QUERYDOCUMENT_HAS_HEADERFIELDSMAP
              headerFieldsMap[fieldName] = valueNode.getText();
#endif
              if ( fieldName == "Content-Length" )
                {
                  contentLength = accu.toUI64();
                  Log_QParse ( "ContentLength : %llu\n", contentLength );
                }
              else if ( fieldName == "Transfer-Encoding" && accu == "chunked" )
                {
                  Log_QParse ( "TransferEncoding chuncked !\n" );
                  transferEncodingChunked = true;
                }
              headerElement = ElementRef(*this);
              accu = "";
              state = QParseState_BeginLine;
              continue;
            }
          accu.appendUtf8(r);
          break;
        case QParseState_CookieStart:
          if ( r == ' ' )
            continue;
          accu.appendUtf8(r);
          state = QParseState_CookieName;
          break;
        case QParseState_CookieName:
          if ( r == '=' )
            {
              Log_QParse ( "Cookie name : '%s'\n", accu.c_str() );
              cookieElement = createElement ( cookiesList, xem_web.cookie() );
              cookieElement.addAttr ( xem_web.name(), accu );
              cookiesList.appendLastChild ( cookieElement );

              accu = "";
              state = QParseState_CookieValue;
              continue;
            }
          accu.appendUtf8(r);
          break;
        case QParseState_CookieValue:
          if ( r == '\r' )
            continue;
          if ( r == ';' || r == '\n' )
            {
              Log_QParse ( "Cookie value : '%s'\n", accu.c_str() );
              ElementRef cookieValueNode = createTextNode ( cookieElement, accu );
              cookieElement.appendLastChild ( cookieValueNode );

              accu = "";
              if ( r == ';' )
                state = QParseState_CookieStart;
              else
                state = QParseState_BeginLine;
              continue;
            }
          accu.appendUtf8(r);
          break;
        default:
          Bug ( "Case %d Not implemented !\n", state );
        }
      }
    if ( state != QParseState_FinishedHTTPHeader )
      {
        throwException ( Exception, "HTTP reader not finished ! state=%d\n", state );
      }
    if ( contentLength || transferEncodingChunked )
      {
        if ( !isQuery || method == "POST" )
          {
            ElementRef content = createElement ( queryElement, xem_web.content() );
            queryElement.appendLastChild ( content );
            BlobRef blob = content.addBlob(xem_web.blob_contents());
            if ( contentLength )
              parseToBlob(blob, contentLength);
            else
              parseChunkedToBlob(blob);
          }
        else
          {
            throwException ( Exception, "Invalid content-length with method = %s\n", method.c_str() );
          }
      }
  }
}
