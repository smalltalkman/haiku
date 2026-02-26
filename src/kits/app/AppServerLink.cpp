/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>

#include <ApplicationPrivate.h>
#include <AppServerLink.h>


/**	AppServerLink provides proxied access to the application's
 *	connection with the app_server.
 *	It has BAutolock semantics:
 *	creating one locks the app_server connection; destroying one
 *	unlocks the connection.
 */


namespace BPrivate {

AppServerLink::AppServerLink()
{
	// if there is no be_app, we can't do a whole lot, anyway
	if (be_app != NULL) {
		BApplication::Private::ServerLink()->Lock();
		fReceiver = &BApplication::Private::ServerLink()->Receiver();
		fSender = &BApplication::Private::ServerLink()->Sender();
	} else {
		debugger("You need to have a valid app_server connection first!");
	}
}


AppServerLink::~AppServerLink()
{
	BApplication::Private::ServerLink()->Unlock();
}

}	// namespace BPrivate
