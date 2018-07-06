/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
#pragma once

#include <memcached/engine_error.h>
#include <memcached/protocol_binary.h>
#include <memcached/types.h>
#include <memcached/vbucket.h>
#include <memcached/visibility.h>

struct DocKey;

/**
 * The message producers are used by the engine's DCP producer
 * to add messages into the DCP stream.  Please look at the full
 * DCP documentation to figure out the real meaning for all of the
 * messages.
 *
 * The DCP client is free to call this functions multiple times
 * to add more messages into the pipeline as long as the producer
 * returns ENGINE_WANT_MORE.
 */
struct dcp_message_producers {
    virtual ~dcp_message_producers() = default;

    virtual ENGINE_ERROR_CODE get_failover_log(uint32_t opaque,
                                               uint16_t vbucket) = 0;

    virtual ENGINE_ERROR_CODE stream_req(uint32_t opaque,
                                         uint16_t vbucket,
                                         uint32_t flags,
                                         uint64_t start_seqno,
                                         uint64_t end_seqno,
                                         uint64_t vbucket_uuid,
                                         uint64_t snap_start_seqno,
                                         uint64_t snap_end_seqno) = 0;

    virtual ENGINE_ERROR_CODE add_stream_rsp(uint32_t opaque,
                                             uint32_t stream_opaque,
                                             uint8_t status) = 0;

    virtual ENGINE_ERROR_CODE marker_rsp(uint32_t opaque, uint8_t status) = 0;

    virtual ENGINE_ERROR_CODE set_vbucket_state_rsp(uint32_t opaque,
                                                    uint8_t status) = 0;

    /**
     * Send a Stream End message
     *
     * @param cookie passed on the cookie provided by step
     * @param opaque this is the opaque requested by the consumer
     *               in the Stream Request message
     * @param vbucket the vbucket id the message belong to
     * @param flags the reason for the stream end.
     *              0 = success
     *              1 = Something happened on the vbucket causing
     *                  us to abort it.
     *
     * @return ENGINE_SUCCESS upon success
     *         ENGINE_EWOULDBLOCK if no data is available
     *         ENGINE_* for errors
     */
    virtual ENGINE_ERROR_CODE stream_end(uint32_t opaque,
                                         uint16_t vbucket,
                                         uint32_t flags) = 0;

    /**
     * Send a marker
     *
     * @param cookie passed on the cookie provided by step
     * @param opaque this is the opaque requested by the consumer
     *               in the Stream Request message
     * @param vbucket the vbucket id the message belong to
     *
     * @return ENGINE_WANT_MORE or ENGINE_SUCCESS upon success
     */
    virtual ENGINE_ERROR_CODE marker(uint32_t opaque,
                                     uint16_t vbucket,
                                     uint64_t start_seqno,
                                     uint64_t end_seqno,
                                     uint32_t flags) = 0;

    /**
     * Send a Mutation
     *
     * @param cookie passed on the cookie provided by step
     * @param opaque this is the opaque requested by the consumer
     *               in the Stream Request message
     * @param itm the item to send. The core will call item_release on
     *            the item when it is sent so remember to keep it around
     * @param vbucket the vbucket id the message belong to
     * @param by_seqno
     * @param rev_seqno
     * @param lock_time
     * @param meta
     * @param nmeta
     * @param nru the nru field used by ep-engine (may safely be ignored)
     * @param collection_len how many bytes of the key are the collection
     *
     * @return ENGINE_WANT_MORE or ENGINE_SUCCESS upon success
     */
    ENGINE_ERROR_CODE (*mutation)
    (gsl::not_null<const void*> cookie,
     uint32_t opaque,
     item* itm,
     uint16_t vbucket,
     uint64_t by_seqno,
     uint64_t rev_seqno,
     uint32_t lock_time,
     const void* meta,
     uint16_t nmeta,
     uint8_t nru,
     uint8_t collection_len) = nullptr;

    /**
     * Send a deletion
     *
     * @param cookie passed on the cookie provided by step
     * @param opaque this is the opaque requested by the consumer
     *               in the Stream Request message
     * @param itm the item to send. The core will call item_release on
     *            the item when it is sent so remember to keep it around
     * @param vbucket the vbucket id the message belong to
     * @param by_seqno
     * @param rev_seqno
     *
     * @return ENGINE_WANT_MORE or ENGINE_SUCCESS upon success
     */
    ENGINE_ERROR_CODE (*deletion)
    (gsl::not_null<const void*> cookie,
     uint32_t opaque,
     item* itm,
     uint16_t vbucket,
     uint64_t by_seqno,
     uint64_t rev_seqno,
     const void* meta,
     uint16_t nmeta) = nullptr;

    /**
     * Send a deletion with delete_time or collections (or both)
     *
     * @param cookie passed on the cookie provided by step
     * @param opaque this is the opaque requested by the consumer
     *               in the Stream Request message
     * @param itm the item to send. The core will call item_release on
     *            the item when it is sent so remember to keep it around
     * @param vbucket the vbucket id the message belong to
     * @param by_seqno
     * @param rev_seqno
     * @param delete_time the time of the deletion (tombstone creation time)
     * @param collection_len how many bytes of the key are the collection
     *
     * @return ENGINE_WANT_MORE or ENGINE_SUCCESS upon success
     */
    ENGINE_ERROR_CODE(*deletion_v2)
    (gsl::not_null<const void*> cookie,
     uint32_t opaque,
     gsl::not_null<item*> itm,
     uint16_t vbucket,
     uint64_t by_seqno,
     uint64_t rev_seqno,
     uint32_t delete_time,
     uint8_t collection_len) = nullptr;

    /**
     * Send an expiration
     *
     * @param cookie passed on the cookie provided by step
     * @param opaque this is the opaque requested by the consumer
     *               in the Stream Request message
     * @param itm the item to send. The core will call item_release on
     *            the item when it is sent so remember to keep it around
     * @param vbucket the vbucket id the message belong to
     * @param by_seqno
     * @param rev_seqno
     * @param collection_len how many bytes of the key are the collection
     *
     * @return ENGINE_WANT_MORE or ENGINE_SUCCESS upon success
     */
    ENGINE_ERROR_CODE (*expiration)
    (gsl::not_null<const void*> cookie,
     uint32_t opaque,
     item* itm,
     uint16_t vbucket,
     uint64_t by_seqno,
     uint64_t rev_seqno,
     const void* meta,
     uint16_t nmeta,
     uint8_t collection_len) = nullptr;

    /**
     * Send a flush for a single vbucket
     *
     * @param cookie passed on the cookie provided by step
     * @param opaque this is the opaque requested by the consumer
     *               in the Stream Request message
     * @param vbucket the vbucket id the message belong to
     *
     * @return ENGINE_WANT_MORE or ENGINE_SUCCESS upon success
     */
    ENGINE_ERROR_CODE(*flush)
    (gsl::not_null<const void*> cookie,
     uint32_t opaque,
     uint16_t vbucket) = nullptr;

    /**
     * Send a state transition for a vbucket
     *
     * @param cookie passed on the cookie provided by step
     * @param opaque this is the opaque requested by the consumer
     *               in the Stream Request message
     * @param vbucket the vbucket id the message belong to
     * @param state the new state
     *
     * @return ENGINE_WANT_MORE or ENGINE_SUCCESS upon success
     */
    ENGINE_ERROR_CODE(*set_vbucket_state)
    (gsl::not_null<const void*> cookie,
     uint32_t opaque,
     uint16_t vbucket,
     vbucket_state_t state) = nullptr;

    /**
     * Send a noop
     *
     * @param cookie passed on the cookie provided by step
     * @param opaque what to use as the opaque in the buffer
     *
     * @return ENGINE_WANT_MORE or ENGINE_SUCCESS upon success
     */
    ENGINE_ERROR_CODE(*noop)
    (gsl::not_null<const void*> cookie, uint32_t opaque) = nullptr;

    /**
     * Send a buffer acknowledgment
     *
     * @param cookie passed on the cookie provided by step
     * @param opaque this is the opaque requested by the consumer
     *               in the Stream Request message
     * @param vbucket the vbucket id the message belong to
     * @param buffer_bytes the amount of bytes processed
     *
     * @return ENGINE_WANT_MORE or ENGINE_SUCCESS upon success
     */
    ENGINE_ERROR_CODE(*buffer_acknowledgement)
    (gsl::not_null<const void*> cookie,
     uint32_t opaque,
     uint16_t vbucket,
     uint32_t buffer_bytes) = nullptr;

    /**
     * Send a control message to the other end
     *
     * @param cookie passed on the cookie provided by step
     * @param opaque what to use as the opaque in the buffer
     * @param key the identifier for the property to set
     * @param nkey the number of bytes in the key
     * @param value The value for the property (the layout of the
     *              value is defined for the key)
     * @paran nvalue The size of the value
     *
     * @return ENGINE_WANT_MORE or ENGINE_SUCCESS upon success
     */
    ENGINE_ERROR_CODE(*control)
    (gsl::not_null<const void*> cookie,
     uint32_t opaque,
     const void* key,
     uint16_t nkey,
     const void* value,
     uint32_t nvalue) = nullptr;

    /**
     * Send a system event message to the other end
     *
     * @param cookie passed on the cookie provided by step
     * @param opaque what to use as the opaque in the buffer
     * @param vbucket the vbucket the event applies to
     * @param bySeqno the sequence number of the event
     * @param key the system event's key data
     * @param eventData the system event's specific data
     *
     * @return ENGINE_WANT_MORE or ENGINE_SUCCESS upon success
     */
    ENGINE_ERROR_CODE(*system_event)
    (gsl::not_null<const void*> cookie,
     uint32_t opaque,
     uint16_t vbucket,
     mcbp::systemevent::id event,
     uint64_t bySeqno,
     cb::const_byte_buffer key,
     cb::const_byte_buffer eventData) = nullptr;

    /*
     * Send a GetErrorMap message to the other end
     *
     * @param cookie The cookie provided by step
     * @param opaque The opaque to send over
     * @param version The version of the error map
     *
     * @return ENGINE_WANT_MORE or ENGINE_SUCCESS upon success
     */
    ENGINE_ERROR_CODE(*get_error_map)
    (gsl::not_null<const void*> cookie,
     uint32_t opaque,
     uint16_t version) = nullptr;
};

typedef ENGINE_ERROR_CODE (*dcp_add_failover_log)(
        vbucket_failover_t*,
        size_t nentries,
        gsl::not_null<const void*> cookie);

struct MEMCACHED_PUBLIC_CLASS DcpIface {
    /**
     * Called from the memcached core for a DCP connection to allow it to
     * inject new messages on the stream.
     *
     * @param cookie a unique handle the engine should pass on to the
     *               message producers
     * @param producers functions the client may use to add messages to
     *                  the DCP stream
     *
     * @return The appropriate error code returned from the message
     *         producerif it failed, or:
     *         ENGINE_SUCCESS if the engine don't have more messages
     *                        to send at this moment
     *         ENGINE_WANT_MORE if the engine have more data it wants
     *                          to send
     *
     */
    virtual ENGINE_ERROR_CODE step(
            gsl::not_null<const void*> cookie,
            gsl::not_null<dcp_message_producers*> producers) = 0;

    /**
     * Called from the memcached core to open a new DCP connection.
     *
     * @param cookie a unique handle the engine should pass on to the
     *               message producers (typically representing the memcached
     *               connection).
     * @param opaque what to use as the opaque for this DCP connection.
     * @param seqno Unused
     * @param flags bitfield of flags to specify what to open. See DCP_OPEN_XXX
     * @param name Identifier for this connection. Note that the name must be
     *             unique; attempting to (re)connect with a name already in use
     *             will disconnect the existing connection.
     * @param jsonExtras Optional JSON string; which if non-empty can be used
     *                   to further control how data is requested - for example
     *                   to filter to specific collections.
     * @return ENGINE_SUCCESS if the DCP connection was successfully opened,
     *         otherwise error code indicating reason for the failure.
     */
    virtual ENGINE_ERROR_CODE open(gsl::not_null<const void*> cookie,
                                   uint32_t opaque,
                                   uint32_t seqno,
                                   uint32_t flags,
                                   cb::const_char_buffer name,
                                   cb::const_byte_buffer jsonExtras) = 0;

    /**
     * Called from the memcached core to add a vBucket stream to the set of
     * connected streams.
     *
     * @param cookie a unique handle the engine should pass on to the
     *               message producers (typically representing the memcached
     *               connection).
     * @param opaque what to use as the opaque for this DCP connection.
     * @param vbucket The vBucket to stream.
     * @param flags bitfield of flags to specify what to open. See
     *              DCP_ADD_STREAM_FLAG_XXX
     * @return ENGINE_SUCCESS if the DCP stream was successfully opened,
     *         otherwise error code indicating reason for the failure.
     */
    virtual ENGINE_ERROR_CODE add_stream(gsl::not_null<const void*> cookie,
                                         uint32_t opaque,
                                         uint16_t vbucket,
                                         uint32_t flags) = 0;

    /**
     * Called from the memcached core to close a vBucket stream to the set of
     * connected streams.
     *
     * @param cookie a unique handle the engine should pass on to the
     *               message producers (typically representing the memcached
     *               connection).
     * @param opaque what to use as the opaque for this DCP connection.
     * @param vbucket The vBucket to close.
     * @return
     */
    virtual ENGINE_ERROR_CODE close_stream(gsl::not_null<const void*> cookie,
                                           uint32_t opaque,
                                           uint16_t vbucket) = 0;

    /**
     * Callback to the engine that a Stream Request message was received
     */
    virtual ENGINE_ERROR_CODE stream_req(gsl::not_null<const void*> cookie,
                                         uint32_t flags,
                                         uint32_t opaque,
                                         uint16_t vbucket,
                                         uint64_t start_seqno,
                                         uint64_t end_seqno,
                                         uint64_t vbucket_uuid,
                                         uint64_t snap_start_seqno,
                                         uint64_t snap_end_seqno,
                                         uint64_t* rollback_seqno,
                                         dcp_add_failover_log callback) = 0;

    /**
     * Callback to the engine that a get failover log message was received
     */
    virtual ENGINE_ERROR_CODE get_failover_log(
            gsl::not_null<const void*> cookie,
            uint32_t opaque,
            uint16_t vbucket,
            dcp_add_failover_log callback) = 0;

    /**
     * Callback to the engine that a stream end message was received
     */
    virtual ENGINE_ERROR_CODE stream_end(gsl::not_null<const void*> cookie,
                                         uint32_t opaque,
                                         uint16_t vbucket,
                                         uint32_t flags) = 0;

    /**
     * Callback to the engine that a snapshot marker message was received
     */
    virtual ENGINE_ERROR_CODE snapshot_marker(gsl::not_null<const void*> cookie,
                                              uint32_t opaque,
                                              uint16_t vbucket,
                                              uint64_t start_seqno,
                                              uint64_t end_seqno,
                                              uint32_t flags) = 0;

    /**
     * Callback to the engine that a mutation message was received
     *
     * @param cookie The cookie representing the connection
     * @param opaque The opaque field in the message (identifying the stream)
     * @param key The documents key
     * @param value The value to store
     * @param priv_bytes The number of bytes in the value which should be
     *                   allocated from the privileged pool
     * @param datatype The datatype for the incomming item
     * @param cas The documents CAS value
     * @param vbucket The vbucket identifier for the document
     * @param flags The user specified flags
     * @param by_seqno The sequence number in the vbucket
     * @param rev_seqno The revision number for the item
     * @param expiration When the document expire
     * @param lock_time The lock time for the document
     * @param meta The documents meta
     * @param nru The engine's NRU value
     * @return Standard engine error code.
     */
    virtual ENGINE_ERROR_CODE mutation(gsl::not_null<const void*> cookie,
                                       uint32_t opaque,
                                       const DocKey& key,
                                       cb::const_byte_buffer value,
                                       size_t priv_bytes,
                                       uint8_t datatype,
                                       uint64_t cas,
                                       uint16_t vbucket,
                                       uint32_t flags,
                                       uint64_t by_seqno,
                                       uint64_t rev_seqno,
                                       uint32_t expiration,
                                       uint32_t lock_time,
                                       cb::const_byte_buffer meta,
                                       uint8_t nru) = 0;

    /**
     * Callback to the engine that a deletion message was received
     *
     * @param cookie The cookie representing the connection
     * @param opaque The opaque field in the message (identifying the stream)
     * @param key The documents key
     * @param value The value to store
     * @param priv_bytes The number of bytes in the value which should be
     *                   allocated from the privileged pool
     * @param datatype The datatype for the incomming item
     * @param cas The documents CAS value
     * @param vbucket The vbucket identifier for the document
     * @param by_seqno The sequence number in the vbucket
     * @param rev_seqno The revision number for the item
     * @param meta The documents meta
     * @return Standard engine error code.
     */
    virtual ENGINE_ERROR_CODE deletion(gsl::not_null<const void*> cookie,
                                       uint32_t opaque,
                                       const DocKey& key,
                                       cb::const_byte_buffer value,
                                       size_t priv_bytes,
                                       uint8_t datatype,
                                       uint64_t cas,
                                       uint16_t vbucket,
                                       uint64_t by_seqno,
                                       uint64_t rev_seqno,
                                       cb::const_byte_buffer meta) = 0;

    /**
     * Callback to the engine that a deletion_v2 message was received
     *
     * @param cookie The cookie representing the connection
     * @param opaque The opaque field in the message (identifying the stream)
     * @param key The documents key
     * @param value The value to store
     * @param priv_bytes The number of bytes in the value which should be
     *                   allocated from the privileged pool
     * @param datatype The datatype for the incomming item
     * @param cas The documents CAS value
     * @param vbucket The vbucket identifier for the document
     * @param by_seqno The sequence number in the vbucket
     * @param rev_seqno The revision number for the item
     * @param delete_time The time of the delete
     * @return Standard engine error code.
     */
    virtual ENGINE_ERROR_CODE deletion_v2(gsl::not_null<const void*> cookie,
                                          uint32_t opaque,
                                          const DocKey& key,
                                          cb::const_byte_buffer value,
                                          size_t priv_bytes,
                                          uint8_t datatype,
                                          uint64_t cas,
                                          uint16_t vbucket,
                                          uint64_t by_seqno,
                                          uint64_t rev_seqno,
                                          uint32_t delete_time) {
        return ENGINE_ENOTSUP;
    }

    /**
     * Callback to the engine that an expiration message was received
     *
     * @param cookie The cookie representing the connection
     * @param opaque The opaque field in the message (identifying the stream)
     * @param key The documents key
     * @param value The value to store
     * @param priv_bytes The number of bytes in the value which should be
     *                   allocated from the privileged pool
     * @param datatype The datatype for the incomming item
     * @param cas The documents CAS value
     * @param vbucket The vbucket identifier for the document
     * @param by_seqno The sequence number in the vbucket
     * @param rev_seqno The revision number for the item
     * @param meta The documents meta
     * @return Standard engine error code.
     */
    virtual ENGINE_ERROR_CODE expiration(gsl::not_null<const void*> cookie,
                                         uint32_t opaque,
                                         const DocKey& key,
                                         cb::const_byte_buffer value,
                                         size_t priv_bytes,
                                         uint8_t datatype,
                                         uint64_t cas,
                                         uint16_t vbucket,
                                         uint64_t by_seqno,
                                         uint64_t rev_seqno,
                                         cb::const_byte_buffer meta) = 0;

    /**
     * Callback to the engine that a flush message was received
     */
    virtual ENGINE_ERROR_CODE flush(gsl::not_null<const void*> cookie,
                                    uint32_t opaque,
                                    uint16_t vbucket) = 0;

    /**
     * Callback to the engine that a set vbucket state message was received
     */
    virtual ENGINE_ERROR_CODE set_vbucket_state(
            gsl::not_null<const void*> cookie,
            uint32_t opaque,
            uint16_t vbucket,
            vbucket_state_t state) = 0;

    /**
     * Callback to the engine that a NOOP message was received
     */
    virtual ENGINE_ERROR_CODE noop(gsl::not_null<const void*> cookie,
                                   uint32_t opaque) = 0;

    /**
     * Callback to the engine that a buffer_ack message was received
     */
    virtual ENGINE_ERROR_CODE buffer_acknowledgement(
            gsl::not_null<const void*> cookie,
            uint32_t opaque,
            uint16_t vbucket,
            uint32_t buffer_bytes) = 0;

    /**
     * Callback to the engine that a Control message was received.
     *
     * @param cookie The cookie representing the connection
     * @param opaque The opaque field in the message (identifying the stream)
     * @param key The control message name (ptr)
     * @param nkey The control message name length
     * @param value The control message value (ptr)
     * @param nvalue The control message value length
     * @return Standard engine error code.
     */
    virtual ENGINE_ERROR_CODE control(gsl::not_null<const void*> cookie,
                                      uint32_t opaque,
                                      const void* key,
                                      uint16_t nkey,
                                      const void* value,
                                      uint32_t nvalue) = 0;

    /**
     * Callback to the engine that a response message has been received.
     * @param cookie The cookie representing the connection
     * @param response The response which the server received.
     * @return Standard engine error code.
     */
    virtual ENGINE_ERROR_CODE response_handler(
            gsl::not_null<const void*> cookie,
            const protocol_binary_response_header* response) = 0;

    /**
     * Callback to the engine that a system event message was received.
     *
     * @param cookie The cookie representing the connection
     * @param opaque The opaque field in the message (identifying the stream)
     * @param vbucket The vbucket identifier for this event.
     * @param event The type of system event.
     * @param bySeqno Sequence number of event.
     * @param key The event name .
     * @param eventData The event value.
     * @return Standard engine error code.
     */
    virtual ENGINE_ERROR_CODE system_event(gsl::not_null<const void*> cookie,
                                           uint32_t opaque,
                                           uint16_t vbucket,
                                           mcbp::systemevent::id event,
                                           uint64_t bySeqno,
                                           cb::const_byte_buffer key,
                                           cb::const_byte_buffer eventData) = 0;
};
