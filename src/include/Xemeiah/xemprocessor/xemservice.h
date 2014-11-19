/*
 * xemservice.h
 *
 *  Created on: 5 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_XEMSERVICE_H_
#define __XEM_XEMSERVICE_H_

#include <Xemeiah/kern/service.h>

namespace Xem
{
  /**
   * Xem XemService API : extended Service handling XProcessor and XemProcessor stuff
   */
  class XemService : public Service
  {
  protected:
    /**
     * Configuration Element : XemService is based on an ElementRef for configuration
     */
    ElementRef configurationElement;

    /**
     * Set default code scope branch
     */
    String defaultCodeScopeBranchName;

    /**
     * Bind a CodeScope to a running XProcessor
     */
    void bindCodeScopeDocument ( XProcessor& xproc, const String& codeScopeBranchName );

    /**
     * Configuration : set current code scope
     */
    virtual String getXProcessorDefaultCodeScopeBranchName ();

    /**
     * Initialize XProcessor
     */
    virtual void initXProcessor ( XProcessor& xproc );

    /**
     * Get my default CurrentCodeScope (which will be configurationElement)
     */
    virtual ElementRef getXProcessorDefaultCurrentNode ();

    /**
     *
     */
    virtual void start ();

    /**
     *
     */
    virtual void stop ();

    /**
     * Call the start thread
     */
    void callStartThread ( );


  public:
    /**
     * Map for startMethodThread arguments
     */
    typedef std::map<KeyId,String> StartMethodThreadArgumentsMap;

  protected:
    /**
     * Call the designated method in the current thread
     * @param brId the branch revId of the current code scope to instanciate
     * @param thisId the ElementId of the element to set as this
     * @param methodId KeyId of the method to call
     * @param args a mapping to the arguments to pass to function
     */
    void callStartMethodThread ( BranchRevId brId, ElementId thisId, LocalKeyId methodId, StartMethodThreadArgumentsMap* args );

  public:
#if 0
    /**
     * Constructor
     */
    XemService ( const ElementRef& configurationElement ) DEPRECATED;
#endif

    /**
     * Constructor with defaults from a XProcessor instance
     */
    XemService ( XProcessor& xproc, const ElementRef& configurationElement );

    /**
     * Virtual destructor
     */
    virtual ~XemService();

    /**
     * Get a reference to our Store
     */
    Store& getStore() const;

    /**
     * Get the configuration Element I am based upon
     */
    ElementRef& getConfigurationElement();

    /**
     * Register myself to the Store
     */
    void registerMyself ( XProcessor& xproc );

    /**
     * Start a thread with a methodId
     */
    void startMethodThread ( const ElementRef& item, LocalKeyId methodId, StartMethodThreadArgumentsMap* argsMap );
  };

  /**
   * Class XemServiceGeneric : simple HACK XemService with a forced XProcessorDefaultCodeScopeBranchName
   */
  class XemServiceGeneric : public XemService
  {
  protected:
    virtual void start () { setStarted(); }
    virtual void stop () {}
  public:
    XemServiceGeneric ( XProcessor& xproc, const ElementRef& configurationElement )
    : XemService ( xproc, configurationElement ) {}

    ~XemServiceGeneric () {}


  };
};

#endif /* __XEM_XEMSERVICE_H_ */
