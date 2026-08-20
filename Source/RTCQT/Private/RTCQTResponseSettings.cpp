#include "RTCQTResponseSettings.h"

FRTCQTResponseParameters URTCQTResponseSettings::MakeProcessorParameters() const
{
	FRTCQTResponseParameters Parameters;
	Parameters.NoiseFloor = NoiseFloor;
	Parameters.InputCeiling = InputCeiling;
	Parameters.ResponseExponent = ResponseExponent;
	Parameters.BandWeights = BandWeights;
	return Parameters;
}
