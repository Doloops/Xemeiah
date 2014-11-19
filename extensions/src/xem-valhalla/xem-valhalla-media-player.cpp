/*
 * xem-valhalla-media-player.cpp
 *
 *  Created on: 23 déc. 2009
 *      Author: francois
 */

#include <Xemeiah/xem-valhalla/xem-valhalla.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/kern/documentref.h>
#include <Xemeiah/xemprocessor/xemservice.h>
#include <Xemeiah/xemprocessor/xemservicemodule.h>
#include <Xemeiah/xemfs/xemfsmodule.h> // For xem-media namespace
#include <Xemeiah/kern/servicemanager.h>
#include <Xemeiah/kern/volatiledocument.h>

#include <Xemeiah/auto-inline.hpp>

#include <player.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include <list>
#include <map>

#define Log_MP Log

namespace Xem
{
  class XemValhallaMediaPlayerService : public XemService
  {
  protected:
    ElementRef* status;
    player_t *player;
    sem_t eventSemaphore;

    virtual void start ();
    virtual void stop ();

    void eventThread ( );
    Mutex eventMutex;
    std::list<player_event_t> eventList;

    std::map<mrl_t*,String> mrlUrlMapping;
    std::list<mrl_t*> mrl_playlist;

    Integer getMRLPosition (mrl_t* mrl);

    void triggerPositionEvent ( Integer pos );

    void setCurrentTitlePosition ();

    virtual void postStop ();
  public:
    __BUILTIN_NAMESPACE_CLASS(xem_media) xem_media;

    XemValhallaMediaPlayerService ( XProcessor& xproc, const ElementRef& configurationElement );

    ~XemValhallaMediaPlayerService ( );

    void sendCommand ( const String& command, const String& path );

    int eventCallback ( player_event_t e, void *data );

    ElementRef getStatus();
  };

  XemValhallaMediaPlayerService::XemValhallaMediaPlayerService ( XProcessor& xproc, const ElementRef& configurationElement )
  : XemService(xproc, configurationElement),xem_media(configurationElement.getDocument().getStore().getKeyCache())
  {
    Log_MP ( "Constructor : Media Player !\n" );
    player = NULL;
    status = NULL;
    sem_init(&eventSemaphore, 0, 0);

  }

  XemValhallaMediaPlayerService::~XemValhallaMediaPlayerService ( )
  {
    Log_MP ( "Destructor : Media Player !\n" );
  }

  /**
   * Awfull hack because libplayer has no way to carry a pointer
   */
  static XemValhallaMediaPlayerService* __staticXemValhallaMediaPlayerService = NULL;
  static int
  __valhalla_event_cb (player_event_t e, void *data)
  {
    AssertBug ( __staticXemValhallaMediaPlayerService, "No __staticXemValhallaMediaPlayerService !!\n" );
    return __staticXemValhallaMediaPlayerService->eventCallback(e, data);
  }

  void XemValhallaMediaPlayerService::start ()
  {
    Log_MP ( "Starting Media Player !\n" );
    AssertBug ( player == NULL, "Already have a player initialized !\n" );
    AssertBug ( status == NULL, "Already have a status element !\n" );
    AssertBug ( __staticXemValhallaMediaPlayerService == NULL, "Already have a MediaPlayer instanciated !\n" );
    __staticXemValhallaMediaPlayerService = this;

    Document* doc = getStore().createVolatileDocument();
    doc->incrementRefCount();
    ElementRef docRoot = doc->getRootElement();
    ElementRef _status = docRoot.getDocument().createElement(docRoot, xem_media.status());
    status = new ElementRef(_status);
    status->addAttr(xem_media.play_state(),"Initialized");
    status->addAttrAsInteger(xem_media.length(),0);
    status->addAttrAsInteger(xem_media.position(),0);
    status->addAttrAsInteger(xem_media.title_position(),0);


    player_type_t player_type = PLAYER_TYPE_VLC;
    player_vo_t player_vo_type = PLAYER_VO_X11;
    // player_ao_t player_ao_type = PLAYER_AO_ALSA;
    player_ao_t player_ao_type = PLAYER_AO_OSS;
    player_verbosity_level_t player_verbosity = PLAYER_MSG_VERBOSE;
    unsigned long windid = 0;

    player_init_param_t playerInitParams;
    playerInitParams.ao = player_ao_type;
    playerInitParams.vo = player_vo_type;
    playerInitParams.winid = windid;
    playerInitParams.event_cb = __valhalla_event_cb;

    player = player_init(player_type, player_verbosity, &playerInitParams);
    // player_ao_type, player_vo_type, player_verbosity, windid, __valhalla_event_cb );

    player_set_playback(player, PLAYER_PB_AUTO);
    player_set_shuffle(player, 0);
    player_set_loop(player,PLAYER_LOOP_DISABLE,0);
    // player_video_set_fullscreen(player,1);

    if ( ! player )
      {
        throwException ( Exception, "Could not run Media Player !\n" );
      }

    Log_MP ( "Media player started, player=%p !\n", player );

    startThread ( boost::bind(&XemValhallaMediaPlayerService::eventThread, this));
  }

  void XemValhallaMediaPlayerService::stop ()
  {
    Log_MP ( "Stopping Media Player !\n" );
  }

  void XemValhallaMediaPlayerService::postStop ()
  {
    Log_MP ( "postStop() for Valhalla Media Player...\n" );
    AssertBug ( player, "No player defined !\n" );
    player_uninit(player);
    player = NULL;
    Store& store = status->getDocument().getStore();
    Document* statusDocument = &(status->getDocument());
    delete ( status );
    store.releaseDocument( statusDocument );
  }

  void XemValhallaMediaPlayerService::triggerPositionEvent(Integer pos)
  {
    AttributeRef positionAttr = status->addAttrAsInteger(xem_media.position(),pos);
    // AttributeRef lengthAttr = status->addAttrAsInteger(xem_media.length(),vlcStatus.length);
    AttributeRef lengthAttr = status->findAttr(xem_media.length(), AttributeType_Integer);

    XProcessor& xproc = getPerThreadXProcessor();

    KeyIdList arguments;
    xproc.setAttribute(xem_media.position(), positionAttr);
    arguments.push_back(xem_media.position());

    xproc.setAttribute(xem_media.length(), lengthAttr);
    arguments.push_back(xem_media.length());

    xproc.triggerEvent(xem_media.change_current_position(),arguments);
  }

  static const char* playerStatusNames[] =
      {
          "Unknown", // PLAYER_EVENT_UNKNOWN
          "Playing", // PLAYER_EVENT_PLAYBACK_START
          "Stopped", // PLAYER_EVENT_PLAYBACK_STOP
          "Stopped", // PLAYER_EVENT_PLAYBACK_FINISHED
          "Stopped", // PLAYER_EVENT_PLAYLIST_FINISHED
          "Paused", // PLAYER_EVENT_PLAYBACK_PAUSE
          "Playing", // PLAYER_EVENT_PLAYBACK_UNPAUSE
          NULL
      };

  Integer XemValhallaMediaPlayerService::getMRLPosition (mrl_t* mrl)
  {
    if ( ! mrl )
      {
        throwException ( Exception, "No MRL !\n" );
      }
    bool found = false;
    Integer title_position = 1;
    for ( std::list<mrl_t*>::iterator iter = mrl_playlist.begin() ; iter != mrl_playlist.end() ; iter++ )
      {
        if ( *iter == mrl )
          {
            found = true;
            break;
          }
        title_position ++;
      }
    if ( ! found )
      {
        throwException ( Exception, "Could not get position of mrl=%p\n", mrl );
      }
    return title_position;
  }

  void XemValhallaMediaPlayerService::setCurrentTitlePosition ()
  {
    mrl_t* mrl = player_mrl_get_current(player);
    Integer title_position = getMRLPosition(mrl);
    status->addAttrAsInteger(xem_media.title_position(),title_position);
  }

  void XemValhallaMediaPlayerService::eventThread ()
  {
    Log_MP ( "Event Thread started !\n" );
    setStarted();

    while ( isStarting() )
      {
        usleep( 100 );
      }

    Log_MP ( "Event Thread : Service started !\n" );

    struct timespec ts;
    player_event_t e;
    int res;
    bool hadNews;

    while ( isRunning() )
      {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;
        hadNews = true;

        res = sem_timedwait(&eventSemaphore,&ts);
        if ( res == -1 )
          {
            if ( errno == ETIMEDOUT )
              {
                Log_MP ( "Timedout !\n" );
                if ( status->getAttr(xem_media.play_state()) == "Playing" )
                  {
                    int pos = player_get_time_pos(player);
                    pos /= 1000;
                    Log_MP ( "POS : %d\n", pos );
                    triggerPositionEvent ( pos );
                  }
                continue;
              }
            else if ( errno == EINTR )
              {
                Log_MP ( "Interruped !\n" );
                continue;
              }
            else
              {
                Bug ( "[SESSION-NEWS] Could not timewait : err=%d:%s\n", errno, strerror(errno) );
              }
          }
        Info ( "New Event !\n" );

        eventMutex.lock ();
        e = eventList.front();
        eventList.pop_front();
        eventMutex.unlock ();

        if ( e < 0 || e > PLAYER_EVENT_PLAYBACK_UNPAUSE )
          {
            Bug ( "Invalid event : %d\n", e );
          }
        String statusStr = playerStatusNames[e];

        Log_MP ( "=> Status = '%s'\n", statusStr.c_str() );
        status->addAttr(xem_media.play_state(), statusStr);
        if ( e == PLAYER_EVENT_PLAYBACK_START )
          {
            status->addAttrAsInteger(xem_media.length(),0);
            status->addAttrAsInteger(xem_media.position(),0);
            setCurrentTitlePosition ();
          }

        KeyIdList arguments;
        XProcessor& xproc = getPerThreadXProcessor();
        xproc.triggerEvent(xem_media.change_current_title(),arguments);

      }
    Log_MP ( "Event Thread : Service ended !\n" );
  }

  int XemValhallaMediaPlayerService::eventCallback ( player_event_t e, void *data )
  {
    Log_MP ( "Callback : player=%p, e=%d (data=%p)\n", player, e, data );
    eventMutex.lock();
    eventList.push_back(e);
    eventMutex.unlock();
    sem_post(&eventSemaphore);

    return 0;
  }


  void XemValhallaMediaPlayerService::sendCommand ( const String& command, const String& arg )
  {
    if ( ! isRunning() )
      {
        throwException ( Exception, "Service not running !\n" );
      }
    Log_MP ( "Send cmd=%s, arg=%s !\n", command.c_str(), arg.c_str() );
    if ( command == "enqueue" )
      {
        mrl_resource_local_args_t *args = (mrl_resource_local_args_t*) malloc ( sizeof(mrl_resource_local_args_t) );
        args->location = strdup(arg.c_str());
        args->playlist = 0;

        mrl_t* mrl = mrl_new (player, MRL_RESOURCE_FILE, args);

        if ( mrl )
          {
            mrlUrlMapping[mrl] = stringFromAllocedStr(strdup(arg.c_str()));
            mrl_playlist.push_back(mrl);
            Log_MP ( "Enqueue '%s' => mrl=%p\n", arg.c_str(), mrl );
            player_mrl_append (player, mrl, PLAYER_MRL_ADD_QUEUE);
          }
        else
          {
            Error ( "NO MRL PROVIDED : Could not get : %s\n", arg.c_str() );
            return;
            throwException ( Exception, "Could not get : %s\n", arg.c_str() );
          }
      }
    else if ( command == "play" )
      {
        player_playback_start (player);
      }
    else if ( command == "stop" )
      {
        player_playback_stop (player);
      }
    else if ( command == "pause" )
      {
        player_playback_pause (player);
      }
    else if ( command == "goto" )
      {
        Integer pos = (Integer) strtod(arg.c_str(), NULL);
        if ( pos == 0 || pos > (Integer) mrl_playlist.size() )
          {
            throwException ( Exception, "goto : out of bounds : pos=%lld, size=%lld\n",
                pos, (Integer) mrl_playlist.size() );
          }
        Log_MP ( "Seek to => %lld\n", pos );
        Integer current = getMRLPosition(player_mrl_get_current(player));

        player_playback_stop (player);

        if ( current == pos )
          {
            Log_MP ( "Nothing to do, already set.\n" );
          }
        else if ( current < pos )
          {
            for ( Integer i = current ; i < pos ; i++ )
              {
                player_mrl_next(player);
              }
          }
        else if ( current > pos )
          {

            for ( Integer i = pos ; i < current ; i++ )
              {
                player_mrl_previous(player);
              }
          }
        current = getMRLPosition(player_mrl_get_current(player));
        if ( current != pos )
          {
            throwException ( Exception, "Could not goto : pos=%lld, current=%lld\n", pos, current );
          }
        player_playback_start (player);
      }
    else if ( command == "next" )
      {
        player_mrl_next(player);
      }
    else if ( command == "prev" )
      {
        player_mrl_previous(player);
      }
    else if ( command == "seek" )
      {
        int i = strtod(arg.c_str(), NULL);

        Log_MP ( "Seeking Integer : %d\n", i );
        player_playback_seek(player, i, PLAYER_PB_SEEK_PERCENT );
      }
    else if ( command == "fastforward" || command == "rewind" )
      {
        int gap = 10;
        if ( command == "rewind" )
          gap = -gap;
        player_playback_seek(player, gap, PLAYER_PB_SEEK_RELATIVE );
      }
    else if ( command == "clear" )
      {
        player_mrl_remove_all(player);
        mrl_playlist.clear();
        mrlUrlMapping.clear();
      }
    else
      {
        throwException ( Exception, "Unknown command : '%s'\n", command.c_str() );
      }
  }

  ElementRef XemValhallaMediaPlayerService::getStatus()
  {
    AssertBug ( status, "No status !\n" );
    return *status;
  }

  void XemValhallaModule::instructionMediaPlayer ( __XProcHandlerArgs__ )
  {
    XemValhallaMediaPlayerService* service = new XemValhallaMediaPlayerService ( getXProcessor(), item );
    service->registerMyself(getXProcessor());
  }

  void XemValhallaModule::instructionSendCommand ( __XProcHandlerArgs__ )
  {
    XemServiceModule& xemServiceModule = XemServiceModule::getMe ( getXProcessor() );
    String serviceName = item.getEvaledAttr(getXProcessor(),xemServiceModule.xem_service.name());
    Service* service_ = getServiceManager().getService(serviceName);
    if ( ! service_ )
      {
        throwException ( Exception, "Could not get service '%s' !\n", serviceName.c_str() );
      }
    XemValhallaMediaPlayerService* service = dynamic_cast<XemValhallaMediaPlayerService*> ( service_ );
    if ( ! service )
      {
        throwException ( Exception, "Could not cast service !\n" );
      }
    String command = item.getEvaledAttr(getXProcessor(),xem_vh.command());
    String arg = item.getEvaledAttr(getXProcessor(),xem_vh.arg());

    service->sendCommand ( command, arg );
  }

  void XemValhallaModule::functionGetStatus(__XProcFunctionArgs__)
  {
    if ( args.size() != 1 )
      {
        throwException ( Exception, "Invalid number of arguments for xem-vh:get-status('service-name')\n");
      }

    String serviceName = args[0]->toString();
    Service* service_ = getServiceManager().getService(serviceName);
    if ( ! service_ )
      {
        throwException ( Exception, "Could not get service '%s' !\n", serviceName.c_str() );
      }
    XemValhallaMediaPlayerService* service = dynamic_cast<XemValhallaMediaPlayerService*> ( service_ );
    if ( ! service )
      {
        throwException ( Exception, "Could not cast service !\n" );
      }
    ElementRef status = service->getStatus();
    result.pushBack(status);
  }

};
