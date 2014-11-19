/*
 * ajaxmodule.cpp
 *
 *  Created on: 14 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/webserver/ajaxmodule.h>
#include <Xemeiah/webserver/nodeflow-http.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/nodeflow/nodeflow-stream.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  __XProcessorLib_REGISTER_MODULE( WebServer, AjaxModuleForge );

  AjaxModule::AjaxModule ( XProcessor& xproc, AjaxModuleForge& moduleForge )
  : XProcessorModule ( xproc, moduleForge ), xem_ajax(moduleForge.xem_ajax)
  {

  }

  AjaxModule::~AjaxModule ()
  {

  }

  void AjaxModule::ajaxUpdates ( __XProcHandlerArgs__ )
  {
    NodeFlowHTTP& nodeFlow = NodeFlowHTTP::asNodeFlowHTTP ( getNodeFlow() );
    nodeFlow.setOutputFormat("text","utf-8",false,false,false);
    nodeFlow.setContentType("text/plain");

    getXProcessor().processChildren(item);
  }

  /**
   * \todo this function is really inefficient, because we are sending result to nodeflow after that..
   * This is a shame that NodeFlowStream could not serialize this properly
   */
  void AjaxModule::writeProtectedJS ( NodeFlow& nodeFlow, const String& value )
  {
    /*
     * \todo Use BufferedWriter instead, or a dedicated nodeFlow callback
     */
    size_t bigBufferSz = 4096;
    size_t bigBufferIdx = 0;
    char* bigBuffer = (char*) malloc(bigBufferSz);

#define __addChar(__c) bigBuffer[bigBufferIdx++] = __c;

    for ( const char* content = value.c_str() ; *content ; content++ )
      {
        if ( bigBufferIdx  > bigBufferSz - 8 )
          {
            if ( bigBufferSz > ( 4 << 20 ) )
              {
                free ( bigBuffer );
                throwException ( Exception, "Very big buffer : 0x%lx bytes\n", (unsigned long) bigBufferSz );
              }
            bigBufferSz *= 2;
            bigBuffer = (char*) realloc(bigBuffer,bigBufferSz);
            if ( bigBuffer == NULL )
              {
                throwException ( Exception, "OOM : Very big buffer : 0x%lx bytes\n", (unsigned long) bigBufferSz );
              }
          }
        if ( *content == '"' )
          {
            __addChar( '\\' );
            __addChar( '"' );
          }
        else if ( *content == '\n' )
          {
            __addChar( ' ' );
          }
        else if ( *content == '\r' )
          {
          }
        else if ( *content == '\\' )
          {
            __addChar( '\\' );
            __addChar( '\\' );
          }
        else
          {
            __addChar(*content);
          }
      }
    __addChar ( '\0' );
    nodeFlow.appendText(bigBuffer, false);
  }

  void AjaxModule::ajaxUpdate ( __XProcHandlerArgs__ )
  {
    String target = item.getEvaledAttr ( getXProcessor(), xem_ajax.target() );
    bool append = item.getEvaledAttr(getXProcessor(), xem_ajax.append()) == "yes";
    bool cleanup = item.hasAttr(xem_ajax.onCleanupMethod());
    int cleanupId = rand();

    NodeFlow& nodeFlow = getNodeFlow();

    String js;
    if ( append )
      {
        js = "var __appendDiv = document.createElement('div');";
        js += "__appendDiv.innerHTML = ";
      }
    else
      {
        js = "document.getElementById('";
        js += target + "').innerHTML ";
      // js += " += ";
        js += "=";
      }
    js += "\"";
    nodeFlow.appendText(js,false);

    if ( 0 && item.getChild() && item.getChild().isText() )
      {
        if ( item.getChild().getYounger() )
          {
            throwException ( Exception, "Invalid xem-ajax:update with multiple texts !\n" );
          }
        writeProtectedJS(nodeFlow,item.getChild().getText());
      }
    else
      {
        NodeFlowStream nfStream ( getXProcessor() );
        nfStream.setOutputFormat("xml","utf-8",false,false,true);
        nfStream.setForceKeepText(true);
        getXProcessor().setNodeFlow(nfStream);
        getXProcessor().processChildren(item);
        writeProtectedJS(nodeFlow, nfStream.getContents());
      }
    if ( cleanup )
      {
        BufferedWriter writer;
        writer.doPrintf("<div id='__xem_cleanup_div_%d_' style='display: none;'>_</div>", cleanupId );
        nodeFlow.appendText(writer.toString(), false);
      }
    nodeFlow.appendText("\";", false);

    if ( cleanup )
      {
        String cleanupMethod = item.getAttr(xem_ajax.onCleanupMethod());

        BufferedWriter writer;
        ElementRef thisElement = getXProcessor().getVariable(getKeyCache().getBuiltinKeys().nons.this_())->toElement();

        writer.doPrintf("function __xem_cleanup_%d_function(event)"
            "{"
            "  if ( event.target.id == '__xem_cleanup_div_%d_' )"
            "  {"
            "     document.getElementById('%s').removeEventListener('DOMNodeRemoved',__xem_cleanup_%d_function,false);"
            "     xem_ajax_sendQueryURL('xem:method=%s&xem:this=%s&xem:class=%s');"
            "  }"
            "}"
            "document.getElementById('%s').addEventListener('DOMNodeRemoved',__xem_cleanup_%d_function,false);",
            cleanupId, cleanupId,
            target.c_str(), cleanupId,
            cleanupMethod.c_str(), thisElement.generateId().c_str(), thisElement.getKey().c_str(),
            target.c_str(), cleanupId );
        nodeFlow.appendText(writer.toString(),true);
      }
    if ( append )
      {
        js = "var __target = document.getElementById('";
        js += target + "');";
        js +=
            "try"
            "{"
            "   while ( __appendDiv.hasChildNodes() )"
            "   {"
            "       __target.appendChild(__appendDiv.firstChild);"
            "   }"
            "}"
            "catch (e) { alert(e); }";
        nodeFlow.appendText(js,false);
      }
  }

  void AjaxModule::ajaxScript ( __XProcHandlerArgs__ )
  {
    getXProcessor().processChildren(item);
  }


  void AjaxModule::install ()
  {
  }

  void AjaxModuleForge::install ()
  {
    registerHandler(xem_ajax.updates(), &AjaxModule::ajaxUpdates );
    registerHandler(xem_ajax.update(), &AjaxModule::ajaxUpdate );
    registerHandler(xem_ajax.script(), &AjaxModule::ajaxScript );
  }

  void AjaxModuleForge::registerEvents ( Document& doc )
  {

  }

};

