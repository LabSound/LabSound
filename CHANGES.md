
## LabSound now follows SemVer

Following SemVer means that LabSound major versions will change more frequently.
Previously LabSound has introduced API incompatibilities on minor version bumps,
now, those incompatibilities will be major version bumps. minor version bumps are
now reserved for API changes that do not break backwards compatibility, and
patch version bumps are reserved for internal changes that do not affect the
public API.

## Automatic pull nodes have been removed. 

Connect and disconnect any nodes that were automatic pull nodes to the destination
node's "null" input.

Destination nodes now have, in input slot 1, a "null" input. The "null" input is
pulled, but the resulting output bus is not rendered to any further outputs.
Use this to drive computation on non-sound producing nodes, such as nodes that
compute an fft for display.

This new mechanism ensures that the computation of the "null" connected nodes
proceeds perfectly in sync, and with the same semantics as non-automatic nodes.

## DeviceNodes no longer sum across input inlets.

inputBus(0) on a device node is now named "in". All inputs connected to "in"
will be summed. InputBus(1) is now named "null". Null will be pulled, but not
summed to the output.

## AudioContext disconnect API is more explicit

Previously the disconnect method used a heuristic based on input parameters
to determine the appropriate disconnection behavior. disconnect has been
replaced with disconnectInput, which disconnects one input or all inputs to
a node, according to the inlet index parameter, and disconnectNode, which
must receive the two nodes to be disconnected. The disconnection may be
predicated on a specific outlet and inlet, however this predication
functionality is currently not implemented; any connection from the source
node to the destination node will be severed.

## AudioNodeDescriptors data drive parameter and setting creation

Every node is now created from its own AudioNodeDescription. Previously, every
node created its parameters and settings through hard coding. This new system
means that every parameter and setting now shares basic information such as the
default value, and strings describing the node. This results in a significant
memory saving in graphs with many nodes of the same kind.

It also means that tools built with LabSound have easy access to a node's 
description, and a reliable mechanism to construct user interfaces such as 
property inspectors or node graphs.

## channelCount is no longer a concept on AudioNodes.

AudioNodes do not have channels. AudioBusses have channels. A strange design
artifact of WebAudio is the notion that AudioNodes can somehow heuristically
configure themselves based on a witch's brew of a user set channel count on
the node, a so-called channel counting mode, and incompletely specified
notions about when mixing should occur. LabSound takes the stance that
channels are a property of the bus, with few exceptions such as nodes
with a specific role that dictates a channel count, for example, a 
StereoPannerNode steers a monophonic input to stereo. Nodes therefore
in the usual case match the output channel count to the input channel count
as mixdowns and mixups for the most part should be determined by the user,
not a heuristic.

AudioBus now has a setNumberOfChannels function.

WebAudio implementations using LabSound are now responsible for emulating
or interpreting channelCount in their implementation as necessary. A
typical fix to setChannelCount on AudioNode would be to change the
implementation to set the channel count on the output bus instead. If
a case exists where this is insufficient due to an AudioNode automatically
overriding the output bus channelCount, please file an Issue so that
this can be addressed on a case by case basis.

## ContextGraphLock has been removed.

The context graph lock, meant to prevent modification of the processing
graph while the audio callback is processing was vestigial. All graph
topology changes have been rewritten to occur when the ContextRenderLock
is held. For the most part, all heavy operations occur on the main thread,
synchronously, and are atomically swapped on the render callback. If any
of these operations are heavy they could cause an audible glitch. No such
glitching operations are currently known, but if they occur, they should be
reported as Issues. Many functions that previously took a ContextGraphLock
no longer do. Simply omit the parameter to update your code.

## FALLING_SAWTOOTH oscillator mode has been added

A rising sawtooth existed, now a falling sawtooth also exists. This means the
OscillatorType enumeration has changed.

## AudioNodeInput, AudioNodeOutput, AudioNodeSummingJunction have been removed

They have been replaced with tiny container classes, AudioNodeSummingInput,
and AudioNodeNamedOutput. The headers corresponding to the old classes are gone.
All backward pointers and AudioNode cyclic dependencies have been removed.
The audio graph is now easily traversed through depth first recursion, and
there is no pointer spaghetti any more.

## AudioNode addInput and addOutput take a name

addInput and addOutput take a name. various housekeeping functions such
as conformChannelCount, and checkNumberOfChannelsForInput are gone.

## AudioNode input(int index) and output(int index) have been replaced

inputBus(r, i), and outputBus(r, i) replace them. inputBus consolidates
the summingJunction functionality. Asking for inputBus(i) will return a
summingBus if many inputs have been connected to a specific inputBus. The
summing is also automatic, and can be relied on when the inputBus is
obtained.

## processIfNecessary has been removed.

All AudioNode processing has been consolidated into a single algorithm
within AudioNode::pull_inputs. The entire process algorithm has been
rewritten for efficiency and clarity. Previously, pulling the graph yielded
a deep and confusing callstack with many forward and backware bus pointers,
and delegation of processing through many helper classes. All of the
intermediate helper classes have been removed, and debugging an AudioNode
is now extremely easy.

## PanningMode renamed PanningModel

This long-standing typo has been corrected.

## PannerNode notifySourcesConnectedToNode removed

This function was never functional, and no issues have ever been reported.

## setting the curve on WaveShaperNode now has no races

Mutexes protect activating new curves.

## AudioProcessorNode is being deprecated.

Don't subclass new nodes from it

## PWNNode is now subclassed from AudioNode

AudioProcessorNode wasn't helping with useful functionality, and made debugging tricky.

## RecorderNode no longer takes a channelCount

It auto configures to record as many channels as exist on the inputBus.

## lpFiterResonance on SfxrNode renamed lpFilterResonance

long standing unreported typo




