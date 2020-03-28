//***************************************************************************
// Copyright 2007-2020 Universidade do Porto - Faculdade de Engenharia      *
// Laboratório de Sistemas e Tecnologia Subaquática (LSTS)                  *
//***************************************************************************
// This file is part of DUNE: Unified Navigation Environment.               *
//                                                                          *
// Commercial Licence Usage                                                 *
// Licencees holding valid commercial DUNE licences may use this file in    *
// accordance with the commercial licence agreement provided with the       *
// Software or, alternatively, in accordance with the terms contained in a  *
// written agreement between you and Faculdade de Engenharia da             *
// Universidade do Porto. For licensing terms, conditions, and further      *
// information contact lsts@fe.up.pt.                                       *
//                                                                          *
// Modified European Union Public Licence - EUPL v.1.1 Usage                *
// Alternatively, this file may be used under the terms of the Modified     *
// EUPL, Version 1.1 only (the "Licence"), appearing in the file LICENCE.md *
// included in the packaging of this file. You may not use this work        *
// except in compliance with the Licence. Unless required by applicable     *
// law or agreed to in writing, software distributed under the Licence is   *
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF     *
// ANY KIND, either express or implied. See the Licence for the specific    *
// language governing permissions and limitations at                        *
// https://github.com/LSTS/dune/blob/master/LICENCE.md and                  *
// http://ec.europa.eu/idabc/eupl.html.                                     *
//***************************************************************************
// Author: Llewellyn-Fernandes                                              *
//***************************************************************************

// DUNE headers.
#include <DUNE/DUNE.hpp>

namespace Monitors
{
	//! Insert short task description here.
	//!
	//! Insert explanation on task behaviour here.
	//! @author Llewellyn-Fernandes
	namespace WifiLog
	{
		using DUNE_NAMESPACES;
		struct Arguments
		{
			//! Socket to bind to
			uint16_t sock_addr;
		};

		struct Task: public DUNE::Tasks::Task
		{
			//! Constructor.
			//! @param[in] name task name.
			//! @param[in] ctx context.
			//! Task arguments
			Arguments m_args;
			//! UDP socket.
			DUNE::Network::UDPSocket* m_sock = NULL;
			//! UDP message buffer.
			uint8_t m_buf[1024];

			Task(const std::string& name, Tasks::Context& ctx):
			DUNE::Tasks::Task(name, ctx)
			{
				param("UDP Data Port", m_args.sock_addr)
				.defaultValue("11223")
				.minimumValue("0")
				.maximumValue("65535")
				.description("UDP data port to listen");
			}

			//! Update internal state with new parameter values.
			void
			onUpdateParameters(void)
			{
				if (m_sock)
				{
					if (paramChanged(m_args.sock_addr))
					{
						throw RestartNeeded(DTR("restarting to change UDP Port"), 1);
					}
				}
			}

			//! Acquire resources.
			void
			onResourceAcquisition(void)
			{
				m_sock = new DUNE::Network::UDPSocket();
			}

			//! Initialize resources.
			void
			onResourceInitialization(void)
			{
				if (m_sock)
				{
					m_sock->bind(m_args.sock_addr);
				}
			}

			//! Release resources.
			void
			onResourceRelease(void)
			{
				Memory::clear(m_sock);
			}

			void
			checkIncomingData()
			{
				Address dummy;
				try
				{
					if (Poll::poll(*m_sock, 0.01))
					{
						size_t n = m_sock->read(m_buf, sizeof(m_buf), &dummy);

						if (n > 0)
						{
							IMC::DevDataText msg;
							msg.value = std::string((char *) m_buf, n);
							dispatch(msg);
						}
					}
				}
				catch (std::runtime_error& e)
				{
					this->err(DTR("Read error: %s"), e.what());
				}
			}

			//! Main loop.
			void
			onMain(void)
			{
				while (!stopping())
				{
					checkIncomingData();
					waitForMessages(0.5);
				}
			}
		};
	}
}

DUNE_TASK
