#include "EventQueueManager.h"

void dae::EventQueueManager::HandleEventQueues()
{
	if(m_EventWasAdded) //only loop trough the event queues when an event was added to avoid doing redundant work this is called dirty flag pattern
	{
		for (auto& pEventQueue : m_pvEventQueues)
		{
			pEventQueue.second->Update();
		}

		m_EventWasAdded = false;
	}

	//check if there are new queues
	if(!m_pvNewEventQueues.empty())
	{
		//if so add them to m_pvEventQueues
		for(auto& pEventQueue : m_pvNewEventQueues)
		{
			m_pvEventQueues[pEventQueue.first] = std::move(pEventQueue.second);
		}

		//clear m_pvNewEventQueues
		m_pvNewEventQueues.clear();
	}
}