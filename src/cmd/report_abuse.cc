/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/player.h"
#include "mud/server.h"
#include "mud/macro.h"
#include "mud/command.h"
#include "common/streams.h"
#include "common/mail.h"
#include "mud/color.h"
#include "mud/telnet.h"
#include "mud/settings.h"

#include "config.h"

/* BEGIN COMMAND
 *
 * name: report abuse
 * usage: report abuse <message>
 *
 * format: report abuse :0* (60)
 *
 * END COMMAND */

void command_report_abuse (Player* player, String argv[])
{
#ifdef HAVE_SENDMAIL
	// mail address
	String rcpt = SettingsManager.get_abuse_email();
	if (!rcpt) {
		*player << CADMIN "Abuse reporting has been disabled." CNORMAL "\n";
		return;
	}

	// sent bug report
	StringBuffer body;
	body << "# -- HEADER --\n";
	body << "Issue: ABUSE\n";
	body << "Host: " << NetworkManager.get_host() << "\n";
	body << "From: " << player->get_account()->get_id() << "\n";
	body << "About: " << argv[0] << "\n";
	body << "# -- END --\n";
	body << "# -- BODY --\n";
	body << argv[1] << "\n";
	body << "# -- END --\n";
	body << '\0';
	MailMessage msg (rcpt, S("Abuse Report"), body.str());
	msg.send();

	// send message 
	Log::Info << "Player " << player->get_account()->get_id() << " issued an abuse report.";
	*player << "Your abuse report has been sent.\n";
#else // HAVE_SENDMAIL
	*player << CADMIN "Bug reporting has been disabled." CNORMAL "\n";
#endif // HAVE_SENDMAIL
}