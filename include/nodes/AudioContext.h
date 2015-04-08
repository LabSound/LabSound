#pragma once

#ifndef AudioContext_h
#define AudioContext_h

#include "AudioScheduledSourceNode.h"
#include "ConcurrentQueue.h"
#include "ExceptionCodes.h"
#include <set>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>

namespace LabSound
{
	class ContextGraphLock;
	class ContextRenderLock;
}

using namespace LabSound;

namespace WebCore
{

class AudioBuffer;
class AudioDestinationNode;
class AudioListener;
class AudioNode;
class AudioScheduledSourceNode;
class HRTFDatabaseLoader;
class MediaStreamAudioSourceNode;
class AudioNodeInput;
class AudioNodeOutput;

template<class Input, class Output>
struct PendingConnection
{
	PendingConnection(std::shared_ptr<Input> from,
					  std::shared_ptr<Output> to,
					  bool connect)
		: from(from), to(to), connect(connect) {}
	bool connect; // true: connect; false: disconnect
	std::shared_ptr<Input> from;
	std::shared_ptr<Output> to;
};

class AudioContext
{
	friend class LabSound::ContextGraphLock;
	friend class LabSound::ContextRenderLock;
public:

	// This is considering 32 is large enough for multiple channels audio.
	// It is somewhat arbitrary and could be increased if necessary.
	static const int maxNumberOfChannels = 32;

	const char * m_graphLocker;
	const char * m_renderLocker;

	// Realtime Context
	AudioContext();

	// Offline (non-realtime) Context
	AudioContext(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate);

	~AudioContext();

	std::shared_ptr<MediaStreamAudioSourceNode> createMediaStreamSource(LabSound::ContextGraphLock & g, LabSound::ContextRenderLock & r);

	void initHRTFDatabase();

	bool isInitialized() const;

	// Returns true when initialize() was called AND all asynchronous initialization has completed.
	bool isRunnable() const;

	// Eexternal users shouldn't use this; it should be called by LabSound::init()
	// It *is* harmless to call it though, it's just not necessary.
	void lazyInitialize();

	void update(LabSound::ContextGraphLock &);

	void stop(LabSound::ContextGraphLock &);

	void setDestinationNode(std::shared_ptr<AudioDestinationNode> node);

	std::shared_ptr<AudioDestinationNode> destination();

	bool isOfflineContext();

	size_t currentSampleFrame() const;

	double currentTime() const;

	float sampleRate() const;

	AudioListener * listener();

	unsigned long activeSourceCount() const;

	void incrementActiveSourceCount();
	void decrementActiveSourceCount();

	// Called periodically at the end of each render quantum to dereference finished source nodes.
	void derefFinishedSourceNodes(LabSound::ContextGraphLock& r);

	// When a source node has no more processing to do (has finished playing), then it tells the context to dereference it.
	void notifyNodeFinishedProcessing(LabSound::ContextRenderLock &, AudioNode *);

	void handlePreRenderTasks(LabSound::ContextRenderLock &); 	// Called at the START of each render quantum.
	void handlePostRenderTasks(LabSound::ContextRenderLock &); 	// Called at the END of each render quantum.

	// We schedule deletion of all marked nodes at the end of each realtime render quantum.
	void markForDeletion(LabSound::ContextRenderLock & r, AudioNode *);
	void deleteMarkedNodes();

	// AudioContext can pull node(s) at the end of each render quantum even when they are not connected to any downstream nodes.
	// These two methods are called by the nodes who want to add/remove themselves into/from the automatic pull lists.
	void addAutomaticPullNode(std::shared_ptr<AudioNode>);
	void removeAutomaticPullNode(std::shared_ptr<AudioNode>);

	// Called right before handlePostRenderTasks() to handle nodes which need to be pulled even when they are not connected to anything.
	void processAutomaticPullNodes(LabSound::ContextRenderLock &, size_t framesToProcess);

	// Keeps track of the number of connections made.
	void incrementConnectionCount();
	unsigned connectionCount() const 
	{ 
		return m_connectionCount; 
	}

	void connect(std::shared_ptr<AudioNode> from, std::shared_ptr<AudioNode> to);
	void connect(std::shared_ptr<AudioNodeInput> fromInput, std::shared_ptr<AudioNodeOutput> toOutput);

	void disconnect(std::shared_ptr<AudioNode> from, std::shared_ptr<AudioNode> to);
	void disconnect(std::shared_ptr<AudioNode>);
	void disconnect(std::shared_ptr<AudioNodeOutput> toOutput);

	void holdSourceNodeUntilFinished(std::shared_ptr<AudioScheduledSourceNode>);

	std::function<void()> renderingCompletedEvent;

private:

	std::mutex m_graphLock;
	std::mutex m_renderLock;

	std::mutex automaticSourcesMutex;

	bool m_isStopScheduled = false;
	bool m_isInitialized = false;
	bool m_isAudioThreadFinished = false;
	bool m_isOfflineContext = false;
	bool m_isDeletionScheduled = false;
	bool m_automaticPullNodesNeedUpdating = false; 	// m_automaticPullNodesNeedUpdating keeps track if m_automaticPullNodes is modified.

	int m_activeSourceCount = 0; // Number of AudioBufferSourceNodes that are active (playing).
	int m_connectionCount = 0;

	void startRendering();

	void uninitialize(LabSound::ContextGraphLock &);

	// Audio thread is dead. Nobody will schedule node deletion action. Let's do it ourselves.
	void clear();

	void scheduleNodeDeletion(LabSound::ContextRenderLock & g);

	void referenceSourceNode(LabSound::ContextGraphLock&, std::shared_ptr<AudioNode>);
	void dereferenceSourceNode(LabSound::ContextGraphLock&, std::shared_ptr<AudioNode>);

	void handleAutomaticSources();
	void updateAutomaticPullNodes();

	std::shared_ptr<AudioDestinationNode> m_destinationNode;
	std::shared_ptr<AudioListener> m_listener;
	std::shared_ptr<HRTFDatabaseLoader> m_hrtfDatabaseLoader;
	std::shared_ptr<AudioBuffer> m_renderTarget;

	std::vector<std::shared_ptr<AudioNode>> m_finishedNodes;

	std::vector<std::shared_ptr<AudioNode>> m_referencedNodes;

	std::vector<std::shared_ptr<AudioNode>> m_nodesToDelete;
	std::vector<std::shared_ptr<AudioNode>> m_nodesMarkedForDeletion;

	std::set<std::shared_ptr<AudioNode>> m_automaticPullNodes;				// queue for added pull nodes
	std::vector<std::shared_ptr<AudioNode>> m_renderingAutomaticPullNodes; 	// vector of known pull nodes

	std::vector<std::shared_ptr<AudioScheduledSourceNode>> automaticSources;

	std::vector<PendingConnection<AudioNodeInput, AudioNodeOutput>> pendingConnections;
    
    
    typedef PendingConnection<AudioNode, AudioNode> PendingNodeConnection;
    
    class CompareScheduledTime {
    public:
        bool operator()(const PendingNodeConnection& p1, const PendingNodeConnection& p2) {
            if (!p2.from->isScheduledNode())
                return true;
            if (!p1.from->isScheduledNode())
                return false;
            AudioScheduledSourceNode *ap1 = dynamic_cast<AudioScheduledSourceNode*>(p1.from.get());
            AudioScheduledSourceNode *ap2 = dynamic_cast<AudioScheduledSourceNode*>(p2.from.get());
            return ap2->startTime() < ap1->startTime();
        }
    };
    
    std::priority_queue<PendingNodeConnection, std::deque<PendingNodeConnection>, CompareScheduledTime> pendingNodeConnections;
	//std::vector<PendingConnection<AudioNode, AudioNode>> pendingNodeConnections;
};

} // End namespace WebCore

#endif // AudioContext_h
