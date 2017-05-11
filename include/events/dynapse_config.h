#ifndef LIBCAER_EVENTS_DYNAPSECONFIG_H_
#define LIBCAER_EVENTS_DYNAPSECONFIG_H_

#include "libcaer/events/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Shift and mask values for the type and data portions
 * of a special event.
 * Up to 128 types, with 24 bits of data each, are possible.
 * Bit 0 is the valid mark, see 'common.h' for more details.
 */
//@{
#define DYNAPSECONFIG_DATA_SHIFT 1
#define DYNAPSECONFIG_CHIPID_SHIFT 0
#define DYNAPSECONFIG_CHIPID_MASK 0x0F
#define DYNAPSECONFIG_DATA_MASK 0xFFFFFFFF

//@}

/**
 * Special event data structure definition.
 * This contains the actual data, as well as the 32 bit event timestamp.
 * Signed integers are used for fields that are to be interpreted
 * directly, for compatibility with languages that do not have
 * unsigned integer types, such as Java.
 */
PACKED_STRUCT(
struct caer_dynapseconfig_event {
	/// Event data. First because of valid mark.
	uint32_t data;
	uint8_t chipid;
	int32_t timestamp;
	/// Event timestamp.
});

/**
 * Type for pointer to special event data structure.
 */
typedef struct caer_dynapseconfig_event *caerDynapseconfig;
typedef const struct caer_dynapseconfig_event *caerDynapseconfigConst;

/**
 * Special event packet data structure definition.
 * EventPackets are always made up of the common packet header,
 * followed by 'eventCapacity' events. Everything has to
 * be in one contiguous memory block.
 */
PACKED_STRUCT(
struct caer_dynapseconfig_event_packet {
	/// The common event packet header.
	struct caer_event_packet_header packetHeader;
	/// The events array.
	struct caer_dynapseconfig_event events[];
});

/**
 * Type for pointer to special event packet data structure.
 */
typedef struct caer_dynapseconfig_event_packet *caerDynapseconfigPacket;
typedef const struct caer_dynapseconfig_event_packet *caerDynapseconfigPacketConst;

/**
 * Allocate a new special events packet.
 * Use free() to reclaim this memory.
 *
 * @param eventCapacity the maximum number of events this packet will hold.
 * @param eventSource the unique ID representing the source/generator of this packet.
 * @param tsOverflow the current timestamp overflow counter value for this packet.
 *
 * @return a valid DynapseconfigPacket handle or NULL on error.
 */
caerDynapseconfigPacket caerDynapseconfigPacketAllocate(int32_t eventCapacity, int16_t eventSource, int32_t tsOverflow);

/**
 * Get the special event at the given index from the event packet.
 *
 * @param packet a valid DynapseconfigPacket pointer. Cannot be NULL.
 * @param n the index of the returned event. Must be within [0,eventCapacity[ bounds.
 *
 * @return the requested special event. NULL on error.
 */
static inline caerDynapseconfig caerDynapseconfigPacketGetEvent(caerDynapseconfigPacket packet, int32_t n) {
	// Check that we're not out of bounds.
	if (n < 0 || n >= caerEventPacketHeaderGetEventCapacity(&packet->packetHeader)) {
		caerLog(CAER_LOG_CRITICAL, "Special Event",
			"Called caerDynapseconfigPacketGetEvent() with invalid event offset %" PRIi32 ", while maximum allowed value is %" PRIi32 ".",
			n, caerEventPacketHeaderGetEventCapacity(&packet->packetHeader) - 1);
		return (NULL);
	}

	// Return a pointer to the specified event.
	return (packet->events + n);
}

/**
 * Get the special event at the given index from the event packet.
 * This is a read-only event, do not change its contents in any way!
 *
 * @param packet a valid DynapseconfigPacket pointer. Cannot be NULL.
 * @param n the index of the returned event. Must be within [0,eventCapacity[ bounds.
 *
 * @return the requested read-only special event. NULL on error.
 */
static inline caerDynapseconfigConst caerDynapseconfigPacketGetEventConst(caerDynapseconfigPacketConst packet, int32_t n) {
	// Check that we're not out of bounds.
	if (n < 0 || n >= caerEventPacketHeaderGetEventCapacity(&packet->packetHeader)) {
		caerLog(CAER_LOG_CRITICAL, "Special Event",
			"Called caerDynapseconfigPacketGetEventConst() with invalid event offset %" PRIi32 ", while maximum allowed value is %" PRIi32 ".",
			n, caerEventPacketHeaderGetEventCapacity(&packet->packetHeader) - 1);
		return (NULL);
	}

	// Return a pointer to the specified event.
	return (packet->events + n);
}

/**
 * Get the 32bit event timestamp, in microseconds.
 * Be aware that this wraps around! You can either ignore this fact,
 * or handle the special 'TIMESTAMP_WRAP' event that is generated when
 * this happens, or use the 64bit timestamp which never wraps around.
 * See 'caerEventPacketHeaderGetEventTSOverflow()' documentation
 * for more details on the 64bit timestamp.
 *
 * @param event a valid Dynapseconfig pointer. Cannot be NULL.
 *
 * @return this event's 32bit microsecond timestamp.
 */
static inline int32_t caerDynapseconfigGetTimestamp(caerDynapseconfigConst event) {
	return (le32toh(event->timestamp));
}

/**
 * Get the 64bit event timestamp, in microseconds.
 * See 'caerEventPacketHeaderGetEventTSOverflow()' documentation
 * for more details on the 64bit timestamp.
 *
 * @param event a valid Dynapseconfig pointer. Cannot be NULL.
 * @param packet the DynapseconfigPacket pointer for the packet containing this event. Cannot be NULL.
 *
 * @return this event's 64bit microsecond timestamp.
 */
static inline int64_t caerDynapseconfigGetTimestamp64(caerDynapseconfigConst event, caerDynapseconfigPacketConst packet) {
	return (I64T(
		(U64T(caerEventPacketHeaderGetEventTSOverflow(&packet->packetHeader)) << TS_OVERFLOW_SHIFT) | U64T(caerDynapseconfigGetTimestamp(event))));
}

/**
 * Set the 32bit event timestamp, the value has to be in microseconds.
 *
 * @param event a valid Dynapseconfig pointer. Cannot be NULL.
 * @param timestamp a positive 32bit microsecond timestamp.
 */
static inline void caerDynapseconfigSetTimestamp(caerDynapseconfig event, int32_t timestamp) {
	if (timestamp < 0) {
		// Negative means using the 31st bit!
		caerLog(CAER_LOG_CRITICAL, "Special Event", "Called caerDynapseconfigSetTimestamp() with negative value!");
		return;
	}

	event->timestamp = htole32(timestamp);
}

/**
 * Check if this special event is valid.
 *
 * @param event a valid Dynapseconfig pointer. Cannot be NULL.
 *
 * @return true if valid, false if not.
 */
static inline bool caerDynapseconfigIsValid(caerDynapseconfigConst event) {
	return (GET_NUMBITS32(event->data, VALID_MARK_SHIFT, VALID_MARK_MASK));
}

/**
 * Validate the current event by setting its valid bit to true
 * and increasing the event packet's event count and valid
 * event count. Only works on events that are invalid.
 * DO NOT CALL THIS AFTER HAVING PREVIOUSLY ALREADY
 * INVALIDATED THIS EVENT, the total count will be incorrect.
 *
 * @param event a valid Dynapseconfig pointer. Cannot be NULL.
 * @param packet the DynapseconfigPacket pointer for the packet containing this event. Cannot be NULL.
 */
static inline void caerDynapseconfigValidate(caerDynapseconfig event, caerDynapseconfigPacket packet) {
	if (!caerDynapseconfigIsValid(event)) {
		SET_NUMBITS32(event->data, VALID_MARK_SHIFT, VALID_MARK_MASK, 1);

		// Also increase number of events and valid events.
		// Only call this on (still) invalid events!
		caerEventPacketHeaderSetEventNumber(&packet->packetHeader,
			caerEventPacketHeaderGetEventNumber(&packet->packetHeader) + 1);
		caerEventPacketHeaderSetEventValid(&packet->packetHeader,
			caerEventPacketHeaderGetEventValid(&packet->packetHeader) + 1);
	}
	else {
		caerLog(CAER_LOG_CRITICAL, "Special Event", "Called caerDynapseconfigValidate() on already valid event.");
	}
}

/**
 * Invalidate the current event by setting its valid bit
 * to false and decreasing the number of valid events held
 * in the packet. Only works with events that are already
 * valid!
 *
 * @param event a valid Dynapseconfig pointer. Cannot be NULL.
 * @param packet the DynapseconfigPacket pointer for the packet containing this event. Cannot be NULL.
 */
static inline void caerDynapseconfigInvalidate(caerDynapseconfig event, caerDynapseconfigPacket packet) {
	if (caerDynapseconfigIsValid(event)) {
		CLEAR_NUMBITS32(event->data, VALID_MARK_SHIFT, VALID_MARK_MASK);

		// Also decrease number of valid events. Number of total events doesn't change.
		// Only call this on valid events!
		caerEventPacketHeaderSetEventValid(&packet->packetHeader,
			caerEventPacketHeaderGetEventValid(&packet->packetHeader) - 1);
	}
	else {
		caerLog(CAER_LOG_CRITICAL, "Special Event", "Called caerDynapseconfigInvalidate() on already invalid event.");
	}
}

/**
 * Get the special event data. Its meaning depends on the type.
 * Current types that make use of it are (see 'enum CAER_DYNAPSECONFIG_event_types'):
 * - DVS_ROW_ONLY: encodes the address of the row from the row-only event.
 *
 * @param event a valid Dynapseconfig pointer. Cannot be NULL.
 *
 * @return the special event data.
 */
static inline uint32_t caerDynapseconfigGetData(caerDynapseconfigConst event) {
	return U32T(GET_NUMBITS32(event->data, DYNAPSECONFIG_DATA_SHIFT, DYNAPSECONFIG_DATA_MASK));
}

static inline uint8_t caerDynapseconfigGetChipid(caerDynapseconfigConst event) {
	return U8T(GET_NUMBITS8(event->chipid, DYNAPSECONFIG_CHIPID_SHIFT, DYNAPSECONFIG_CHIPID_MASK));
}

/**
 * Set the special event data. Its meaning depends on the type.
 * Current types that make use of it are (see 'enum CAER_DYNAPSECONFIG_event_types'):
 * - DVS_ROW_ONLY: encodes the address of the row from the row-only event.
 *
 * @param event a valid Dynapseconfig pointer. Cannot be NULL.
 * @param data the special event data.
 */
static inline void caerDynapseconfigSetData(caerDynapseconfig event, uint32_t data) {
	CLEAR_NUMBITS32(event->data, DYNAPSECONFIG_DATA_SHIFT, DYNAPSECONFIG_DATA_MASK);
	SET_NUMBITS32(event->data, DYNAPSECONFIG_DATA_SHIFT, DYNAPSECONFIG_DATA_MASK, data);
}

static inline void caerDynapseconfigSetChipid(caerDynapseconfig event, uint8_t chipid) {
	CLEAR_NUMBITS8(event->chipid, DYNAPSECONFIG_CHIPID_SHIFT, DYNAPSECONFIG_CHIPID_MASK);
	SET_NUMBITS8(event->chipid, DYNAPSECONFIG_CHIPID_SHIFT, DYNAPSECONFIG_CHIPID_MASK, chipid);
}

/**
 * Iterator over all special events in a packet.
 * Returns the current index in the 'caerDynapseconfigIteratorCounter' variable of type
 * 'int32_t' and the current event in the 'caerDynapseconfigIteratorElement' variable
 * of type caerDynapseconfig.
 *
 * DYNAPSECONFIG_PACKET: a valid DynapseconfigPacket pointer. Cannot be NULL.
 */
#define CAER_DYNAPSECONFIG_ITERATOR_ALL_START(DYNAPSECONFIG_PACKET) \
	for (int32_t caerDynapseconfigIteratorCounter = 0; \
		caerDynapseconfigIteratorCounter < caerEventPacketHeaderGetEventNumber(&(DYNAPSECONFIG_PACKET)->packetHeader); \
		caerDynapseconfigIteratorCounter++) { \
		caerDynapseconfig caerDynapseconfigIteratorElement = caerDynapseconfigPacketGetEvent(DYNAPSECONFIG_PACKET, caerDynapseconfigIteratorCounter);

/**
 * Const-Iterator over all special events in a packet.
 * Returns the current index in the 'caerDynapseconfigIteratorCounter' variable of type
 * 'int32_t' and the current read-only event in the 'caerDynapseconfigIteratorElement' variable
 * of type caerDynapseconfigConst.
 *
 * DYNAPSECONFIG_PACKET: a valid DynapseconfigPacket pointer. Cannot be NULL.
 */
#define CAER_DYNAPSECONFIG_CONST_ITERATOR_ALL_START(DYNAPSECONFIG_PACKET) \
	for (int32_t caerDynapseconfigIteratorCounter = 0; \
		caerDynapseconfigIteratorCounter < caerEventPacketHeaderGetEventNumber(&(DYNAPSECONFIG_PACKET)->packetHeader); \
		caerDynapseconfigIteratorCounter++) { \
		caerDynapseconfigConst caerDynapseconfigIteratorElement = caerDynapseconfigPacketGetEventConst(DYNAPSECONFIG_PACKET, caerDynapseconfigIteratorCounter);

/**
 * Iterator close statement.
 */
#define CAER_DYNAPSECONFIG_ITERATOR_ALL_END }

/**
 * Iterator over only the valid special events in a packet.
 * Returns the current index in the 'caerDynapseconfigIteratorCounter' variable of type
 * 'int32_t' and the current event in the 'caerDynapseconfigIteratorElement' variable
 * of type caerDynapseconfig.
 *
 * DYNAPSECONFIG_PACKET: a valid DynapseconfigPacket pointer. Cannot be NULL.
 */
#define CAER_DYNAPSECONFIG_ITERATOR_VALID_START(DYNAPSECONFIG_PACKET) \
	for (int32_t caerDynapseconfigIteratorCounter = 0; \
		caerDynapseconfigIteratorCounter < caerEventPacketHeaderGetEventNumber(&(DYNAPSECONFIG_PACKET)->packetHeader); \
		caerDynapseconfigIteratorCounter++) { \
		caerDynapseconfig caerDynapseconfigIteratorElement = caerDynapseconfigPacketGetEvent(DYNAPSECONFIG_PACKET, caerDynapseconfigIteratorCounter); \
		if (!caerDynapseconfigIsValid(caerDynapseconfigIteratorElement)) { continue; } // Skip invalid special events.

/**
 * Const-Iterator over only the valid special events in a packet.
 * Returns the current index in the 'caerDynapseconfigIteratorCounter' variable of type
 * 'int32_t' and the current read-only event in the 'caerDynapseconfigIteratorElement' variable
 * of type caerDynapseconfigConst.
 *
 * DYNAPSECONFIG_PACKET: a valid DynapseconfigPacket pointer. Cannot be NULL.
 */
#define CAER_DYNAPSECONFIG_CONST_ITERATOR_VALID_START(DYNAPSECONFIG_PACKET) \
	for (int32_t caerDynapseconfigIteratorCounter = 0; \
		caerDynapseconfigIteratorCounter < caerEventPacketHeaderGetEventNumber(&(DYNAPSECONFIG_PACKET)->packetHeader); \
		caerDynapseconfigIteratorCounter++) { \
		caerDynapseconfigConst caerDynapseconfigIteratorElement = caerDynapseconfigPacketGetEventConst(DYNAPSECONFIG_PACKET, caerDynapseconfigIteratorCounter); \
		if (!caerDynapseconfigIsValid(caerDynapseconfigIteratorElement)) { continue; } // Skip invalid special events.

/**
 * Iterator close statement.
 */
#define CAER_DYNAPSECONFIG_ITERATOR_VALID_END }

/**
 * Reverse iterator over all special events in a packet.
 * Returns the current index in the 'caerDynapseconfigIteratorCounter' variable of type
 * 'int32_t' and the current event in the 'caerDynapseconfigIteratorElement' variable
 * of type caerDynapseconfig.
 *
 * DYNAPSECONFIG_PACKET: a valid DynapseconfigPacket pointer. Cannot be NULL.
 */
#define CAER_DYNAPSECONFIG_REVERSE_ITERATOR_ALL_START(DYNAPSECONFIG_PACKET) \
	for (int32_t caerDynapseconfigIteratorCounter = caerEventPacketHeaderGetEventNumber(&(DYNAPSECONFIG_PACKET)->packetHeader) - 1; \
		caerDynapseconfigIteratorCounter >= 0; \
		caerDynapseconfigIteratorCounter--) { \
		caerDynapseconfig caerDynapseconfigIteratorElement = caerDynapseconfigPacketGetEvent(DYNAPSECONFIG_PACKET, caerDynapseconfigIteratorCounter);
/**
 * Const-Reverse iterator over all special events in a packet.
 * Returns the current index in the 'caerDynapseconfigIteratorCounter' variable of type
 * 'int32_t' and the current read-only event in the 'caerDynapseconfigIteratorElement' variable
 * of type caerDynapseconfigConst.
 *
 * DYNAPSECONFIG_PACKET: a valid DynapseconfigPacket pointer. Cannot be NULL.
 */
#define CAER_DYNAPSECONFIG_CONST_REVERSE_ITERATOR_ALL_START(DYNAPSECONFIG_PACKET) \
	for (int32_t caerDynapseconfigIteratorCounter = caerEventPacketHeaderGetEventNumber(&(DYNAPSECONFIG_PACKET)->packetHeader) - 1; \
		caerDynapseconfigIteratorCounter >= 0; \
		caerDynapseconfigIteratorCounter--) { \
		caerDynapseconfigConst caerDynapseconfigIteratorElement = caerDynapseconfigPacketGetEventConst(DYNAPSECONFIG_PACKET, caerDynapseconfigIteratorCounter);

/**
 * Reverse iterator close statement.
 */
#define CAER_DYNAPSECONFIG_REVERSE_ITERATOR_ALL_END }

/**
 * Reverse iterator over only the valid special events in a packet.
 * Returns the current index in the 'caerDynapseconfigIteratorCounter' variable of type
 * 'int32_t' and the current event in the 'caerDynapseconfigIteratorElement' variable
 * of type caerDynapseconfig.
 *
 * DYNAPSECONFIG_PACKET: a valid DynapseconfigPacket pointer. Cannot be NULL.
 */
#define CAER_DYNAPSECONFIG_REVERSE_ITERATOR_VALID_START(DYNAPSECONFIG_PACKET) \
	for (int32_t caerDynapseconfigIteratorCounter = caerEventPacketHeaderGetEventNumber(&(DYNAPSECONFIG_PACKET)->packetHeader) - 1; \
		caerDynapseconfigIteratorCounter >= 0; \
		caerDynapseconfigIteratorCounter--) { \
		caerDynapseconfig caerDynapseconfigIteratorElement = caerDynapseconfigPacketGetEvent(DYNAPSECONFIG_PACKET, caerDynapseconfigIteratorCounter); \
		if (!caerDynapseconfigIsValid(caerDynapseconfigIteratorElement)) { continue; } // Skip invalid special events.

/**
 * Const-Reverse iterator over only the valid special events in a packet.
 * Returns the current index in the 'caerDynapseconfigIteratorCounter' variable of type
 * 'int32_t' and the current read-only event in the 'caerDynapseconfigIteratorElement' variable
 * of type caerDynapseconfigConst.
 *
 * DYNAPSECONFIG_PACKET: a valid DynapseconfigPacket pointer. Cannot be NULL.
 */
#define CAER_DYNAPSECONFIG_CONST_REVERSE_ITERATOR_VALID_START(DYNAPSECONFIG_PACKET) \
	for (int32_t caerDynapseconfigIteratorCounter = caerEventPacketHeaderGetEventNumber(&(DYNAPSECONFIG_PACKET)->packetHeader) - 1; \
		caerDynapseconfigIteratorCounter >= 0; \
		caerDynapseconfigIteratorCounter--) { \
		caerDynapseconfigConst caerDynapseconfigIteratorElement = caerDynapseconfigPacketGetEventConst(DYNAPSECONFIG_PACKET, caerDynapseconfigIteratorCounter); \
		if (!caerDynapseconfigIsValid(caerDynapseconfigIteratorElement)) { continue; } // Skip invalid special events.

/**
 * Reverse iterator close statement.
 */
#define CAER_DYNAPSECONFIG_REVERSE_ITERATOR_VALID_END }

/**
 * Get the first special event with the given event type in this
 * event packet. This returns the first found event with that type ID,
 * or NULL if we get to the end without finding any such event.
 *
 * @param packet a valid DynapseconfigPacket pointer. Cannot be NULL.
 * @param type the special event type to search for.
 *
 * @return the requested special event or NULL on error/not found.
 */
static inline caerDynapseconfig caerDynapseconfigPacketFindEventByChipid(caerDynapseconfigPacket packet, uint8_t chipid) {
	CAER_DYNAPSECONFIG_ITERATOR_ALL_START(packet)
		if (caerDynapseconfigGetType(caerDynapseconfigIteratorElement) == chipid) {
			// Found it, return it.
			return (caerDynapseconfigIteratorElement);
		}
	CAER_DYNAPSECONFIG_ITERATOR_ALL_END

	// Found nothing, return nothing.
	return (NULL);
}

/**
 * Get the first special event with the given event type in this
 * event packet. This returns the first found event with that type ID,
 * or NULL if we get to the end without finding any such event.
 * The returned event is read-only!
 *
 * @param packet a valid DynapseconfigPacket pointer. Cannot be NULL.
 * @param type the special event type to search for.
 *
 * @return the requested read-only special event or NULL on error/not found.
 */
static inline caerDynapseconfigConst caerDynapseconfigPacketFindEventByChipidConst(caerDynapseconfigPacketConst packet, uint8_t chipid) {
	CAER_DYNAPSECONFIG_CONST_ITERATOR_ALL_START(packet)
		if (caerDynapseconfigGetType(caerDynapseconfigIteratorElement) == chipid) {
			// Found it, return it.
			return (caerDynapseconfigIteratorElement);
		}
	CAER_DYNAPSECONFIG_ITERATOR_ALL_END

	// Found nothing, return nothing.
	return (NULL);
}

/**
 * Get the first valid special event with the given event type in this
 * event packet. This returns the first found valid event with that type ID,
 * or NULL if we get to the end without finding any such event.
 *
 * @param packet a valid DynapseconfigPacket pointer. Cannot be NULL.
 * @param type the special event type to search for.
 *
 * @return the requested valid special event or NULL on error/not found.
 */
static inline caerDynapseconfig caerDynapseconfigPacketFindValidEventByChipid(caerDynapseconfigPacket packet, uint8_t chipid) {
	CAER_DYNAPSECONFIG_ITERATOR_VALID_START(packet)
		if (caerDynapseconfigGetType(caerDynapseconfigIteratorElement) == chipid) {
			// Found it, return it.
			return (caerDynapseconfigIteratorElement);
		}
	CAER_DYNAPSECONFIG_ITERATOR_VALID_END

	// Found nothing, return nothing.
	return (NULL);
}

/**
 * Get the first valid special event with the given event type in this
 * event packet. This returns the first found valid event with that type ID,
 * or NULL if we get to the end without finding any such event.
 * The returned event is read-only!
 *
 * @param packet a valid DynapseconfigPacket pointer. Cannot be NULL.
 * @param type the special event type to search for.
 *
 * @return the requested read-only valid special event or NULL on error/not found.
 */
static inline caerDynapseconfigConst caerDynapseconfigPacketFindValidEventByTypeConst(caerDynapseconfigPacketConst packet, uint8_t chipid) {
	CAER_DYNAPSECONFIG_CONST_ITERATOR_VALID_START(packet)
		if (caerDynapseconfigGetType(caerDynapseconfigIteratorElement) == chipid) {
			// Found it, return it.
			return (caerDynapseconfigIteratorElement);
		}
	CAER_DYNAPSECONFIG_ITERATOR_VALID_END

	// Found nothing, return nothing.
	return (NULL);
}

#ifdef __cplusplus
}
#endif

#endif /* LIBCAER_EVENTS_DYNAPSECONFIG_H_ */
