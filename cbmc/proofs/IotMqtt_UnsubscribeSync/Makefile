ENTRY=IotMqtt_UnsubscribeSync

################################################################
# Proof assumptions

# Bound the number of subscriptions in a list (BOUND = MAX-1)
SUBSCRIPTION_COUNT_MAX=2
DEF += -DSUBSCRIPTION_COUNT_MAX=$(SUBSCRIPTION_COUNT_MAX)

# Bound the number of operations in a list (BOUND = MAX-1)
OPERATION_COUNT_MAX=2
DEF += -DOPERATION_COUNT_MAX=$(OPERATION_COUNT_MAX)

# Bound the length of subscription topics (BOUND = MAX-1)
TOPIC_LENGTH_MAX=8
DEF += -DTOPIC_LENGTH_MAX=$(TOPIC_LENGTH_MAX)

# A constant that should be 2*SUBSCRIPTION_COUNT_MAX-1
SUBSCRIPTION_LIST_MAX=3
DEF += -DSUBSCRIPTION_LIST_MAX=$(SUBSCRIPTION_LIST_MAX)

################################################################
# MQTT configuration constants modified for testing

# this should be smaller than topic_length_max for coverage
# _validateSubscription coverage at line 385
AWS_IOT_MQTT_SERVER_MAX_TOPIC_LENGTH=6
DEF += -DAWS_IOT_MQTT_SERVER_MAX_TOPIC_LENGTH=$(AWS_IOT_MQTT_SERVER_MAX_TOPIC_LENGTH)

# this should be smaller than topic_length_max for coverage
# _IotMqtt_SubscriptionPacketSize coverage at line 702
MQTT_MAX_REMAINING_LENGTH=6
DEF += -DMQTT_MAX_REMAINING_LENGTH=$(MQTT_MAX_REMAINING_LENGTH)

################################################################
# Function omitted from the proof

ABSTRACTIONS += --remove-function-body IotLog_Generic

# Unreachable functions removed for accurate coverage computation
ABSTRACTIONS += --remove-function-body IotNetworkInterfaceClose
ABSTRACTIONS += --remove-function-body IotNetworkInterfaceReceive
ABSTRACTIONS += --remove-function-body _IotMqtt_AddSubscriptions
ABSTRACTIONS += --remove-function-body _IotMqtt_SerializeSubscribe
ABSTRACTIONS += --remove-function-body _IotMqtt_SerializeSubscribeCommon
ABSTRACTIONS += --remove-function-body _matchEndWildcards
ABSTRACTIONS += --remove-function-body _matchWildcards
ABSTRACTIONS += --remove-function-body _mqttOperation_match
ABSTRACTIONS += --remove-function-body _mqttSubscription_setUnsubscribe
ABSTRACTIONS += --remove-function-body _packetMatch
ABSTRACTIONS += --remove-function-body _topicFilterMatch
ABSTRACTIONS += --remove-function-body _validateQos

# Functions manually determined to be unreachable in their calling context
# See README.md for explanations of unreachability
ABSTRACTIONS += --remove-function-body _IotMqtt_RemoveSubscriptionByPacket
ABSTRACTIONS += --remove-function-body _IotMqtt_ScheduleOperation
ABSTRACTIONS += --remove-function-body _checkRetryLimit
ABSTRACTIONS += --remove-function-body _scheduleCallback
ABSTRACTIONS += --remove-function-body _scheduleNextRetry

################################################################

OBJS += $(ENTRY)_harness.goto
OBJS += $(MQTT)/cbmc/proofs/mqtt_state.goto
OBJS += $(MQTT)/cbmc/stubs/_IotMqtt_NextPacketIdentifier.goto
OBJS += $(MQTT)/cbmc/stubs/IotListDouble_RemoveAllMatches.goto
OBJS += $(MQTT)/cbmc/stubs/IotSemaphore.goto
OBJS += $(MQTT)/cbmc/stubs/IotTaskPool_TryCancel.goto
OBJS += $(MQTT)/libraries/standard/mqtt/src/iot_mqtt_api.goto
OBJS += $(MQTT)/libraries/standard/mqtt/src/iot_mqtt_helper.goto
OBJS += $(MQTT)/libraries/standard/mqtt/src/iot_mqtt_operation.goto
OBJS += $(MQTT)/libraries/standard/mqtt/src/iot_mqtt_serialize.goto
OBJS += $(MQTT)/libraries/standard/mqtt/src/iot_mqtt_subscription.goto
OBJS += $(MQTT)/libraries/standard/mqtt/src/iot_mqtt_validate.goto

DEF += -DCBMC_STUB_REMOVEALLMATCHES=1

# allow mqtt mallocs to fail for coverage
DEF += -include mqtt_state.h
DEF += -DIotMqtt_MallocMessage=malloc_can_fail
DEF += -DIotMqtt_MallocOperation=malloc_can_fail

################################################################
# Loop unwinding

LOOP += IotListDouble_FindFirstMatch.0:$(SUBSCRIPTION_COUNT_MAX)
LOOP += IotListDouble_FindFirstMatch\$$link1.0:$(SUBSCRIPTION_COUNT_MAX)
LOOP += IotListDouble_FindFirstMatch\$$link2.0:$(SUBSCRIPTION_COUNT_MAX)
LOOP += IotListDouble_FindFirstMatch\$$link3.0:$(SUBSCRIPTION_COUNT_MAX)
LOOP += IotListDouble_FindFirstMatch\$$link4.0:$(SUBSCRIPTION_COUNT_MAX)
LOOP += IotListDouble_FindFirstMatch\$$link5.0:$(SUBSCRIPTION_COUNT_MAX)

LOOP += _IotMqtt_RemoveSubscriptionByTopicFilter.0:$(SUBSCRIPTION_COUNT_MAX)
LOOP += _IotMqtt_SerializeUnsubscribeCommon.0:$(SUBSCRIPTION_COUNT_MAX)
LOOP += _IotMqtt_SubscriptionPacketSize.0:$(SUBSCRIPTION_COUNT_MAX)
LOOP += _IotMqtt_ValidateSubscriptionList.0:$(SUBSCRIPTION_COUNT_MAX)
LOOP += _validateSubscription.0:$(TOPIC_LENGTH_MAX)
LOOP += allocate_IotMqttSubscriptionArray.0:$(SUBSCRIPTION_COUNT_MAX)
LOOP += strncmp.0:$(TOPIC_LENGTH_MAX)
LOOP += valid_IotMqttOperationList.0:$(OPERATION_COUNT_MAX)
LOOP += valid_IotMqttSubscriptionArray.0:$(SUBSCRIPTION_COUNT_MAX)
LOOP += valid_IotMqttSubscriptionList.0:$(SUBSCRIPTION_COUNT_MAX)

UNWINDING += --unwind 1
UNWINDING += --unwindset '$(shell echo $(LOOP) | sed 's/ /,/g')'

################################################################

include ../Makefile.common
