/*
 * StreamPusher.h
 *
 * Created on: Feb 08, 2016
 *     Author: Manuel Penschuck (manuel@ae.cs.uni-frankfurt.de)
 *             Hung Tran (hung@ae.cs.uni-frankfurt.de)
 */

#pragma once

/**
 * Connects a producer implemented as a stream to an consumer which accepts
 * elements using a push() method.
 */
template <class InStream, class OutReceiver>
class StreamPusher {
    InStream & _input;
    OutReceiver & _output;

public:
    StreamPusher(InStream & input, OutReceiver & output, bool start_now = true)
        : _input(input), _output(output)
    {
        if (start_now)
            process();
    }

    void process() {
        for(; !_input.empty(); ++_input)
            _output.push(*_input);
    }
};

/**
 * Connects a producer implemented as a stream to an consumer which accepts
 * elements using a push_back() method.
 */
template <class InStream, class OutReceiver>
class StreamBackpusher {
    InStream & _input;
    OutReceiver & _output;

public:
    StreamBackpusher(InStream & input, OutReceiver & output, bool start_now = true)
        : _input(input), _output(output)
    {
        if (start_now)
            process();
    }

    void process() {
        for(; !_input.empty(); ++_input)
            _output.push_back(*_input);
    }
};

template <typename SequenceType, typename PushReceiver>
void push_sequence_to_pushreceiver(SequenceType &sequence, PushReceiver &pushreceiver) {
    for (auto stream = sequence.get_stream(); !stream.empty(); ++stream)
        pushreceiver.push(*stream);
}

template <typename SequenceType, typename PushBackReceiver>
void push_sequence_to_pushbackreceiver(SequenceType &sequence, PushBackReceiver &pushreceiver) {
    for (auto stream = sequence.get_stream(); !stream.empty(); ++stream)
        pushreceiver.push_back(*stream);
}

template <typename StreamType, typename PushReceiver>
void push_stream_to_pushreceiver(StreamType & stream, PushReceiver & pushreceiver) {
    for (; !stream.empty(); ++stream)
        pushreceiver.push(*stream);
}

template <typename StreamType, typename PushReceiver, typename MapFunctor>
void push_stream_to_pushreceiver(StreamType & stream, PushReceiver & pushreceiver, MapFunctor map) {
    for (; !stream.empty(); ++stream)
        pushreceiver.push(map(*stream));
}

template <typename StreamType, typename PushBackReceiver>
void push_stream_to_pushbackreceiver(StreamType & stream, PushBackReceiver & pushbackreceiver) {
    for (; !stream.empty(); ++stream)
        pushbackreceiver.push_back(*stream);
}

template <typename StreamType, typename PushBackReceiver, typename MapFunctor>
void push_stream_to_pushbackreceiver(StreamType & stream, PushBackReceiver & pushbackreceiver, MapFunctor map) {
    for (; !stream.empty(); ++stream)
        pushbackreceiver.push_back(map(*stream));
}

template <typename StreamType, typename PushBackReceiver, typename MapFunctor, typename SentinelFunctor>
void push_stream_to_pushbackreceiver(StreamType & stream, PushBackReceiver & pushbackreceiver, MapFunctor map, SentinelFunctor sentinel) {
    for (; !stream.empty(); ++stream) {
        if (sentinel(*stream))
            break;
        pushbackreceiver.push_back(map(*stream));
    }
}