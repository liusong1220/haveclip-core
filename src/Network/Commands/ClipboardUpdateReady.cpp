/*
  HaveClip

  Copyright (C) 2013-2016 Jakub Skokan <aither@havefun.cz>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ClipboardUpdateReady.h"

using namespace Commands;

ClipboardUpdateReady::ClipboardUpdateReady(ClipboardContainer *cont, Communicator::Role r)
	: BaseCommand(cont, r)
{
}

BaseCommand::Type ClipboardUpdateReady::type() const
{
	return BaseCommand::ClipboardUpdateReady;
}

void ClipboardUpdateReady::receive(QDataStream &ds)
{
	qint32 mode;
	ds >> mode;

	m_mode = (ClipboardContainer::Mode) mode;

	finish();
}

void ClipboardUpdateReady::send(QDataStream &ds)
{
	ds << (qint32) m_cont->mode;

	finish();
}

ClipboardContainer::Mode ClipboardUpdateReady::mode()
{
	return m_mode;
}
