// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Physics/PhysicsInterfaceUtils.h"
#include "PhysicsReplication.h"

#include "Engine/ReplicatedState.h"
#include "PhysicsReplicationInterface.h"
#include "Physics/PhysicsInterfaceDeclares.h"
#include "PhysicsProxy/SingleParticlePhysicsProxyFwd.h"
#include "Chaos/Particles.h"
#include "Chaos/PhysicsObject.h"
#include "Chaos/SimCallbackObject.h"


#include "GrippablePhysicsReplication.generated.h"
//#include "GrippablePhysicsReplication.generated.h"


//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVRPhysicsReplicationDelegate, void, Return);

/*static TAutoConsoleVariable<int32> CVarEnableCustomVRPhysicsReplication(
	TEXT("vr.VRExpansion.EnableCustomVRPhysicsReplication"),
	0,
	TEXT("Enable valves input controller that overrides legacy input.\n")
	TEXT(" 0: use the engines default input mapping (default), will also be default if vr.SteamVR.EnableVRInput is enabled\n")
	TEXT(" 1: use the valve input controller. You will have to define input bindings for the controllers you want to support."),
	ECVF_ReadOnly);*/

//#if PHYSICS_INTERFACE_PHYSX
//struct FAsyncPhysicsRepCallbackDataVR;
//class FPhysicsReplicationAsyncCallbackVR;

class FPhysicsReplicationAsyncVR : public Chaos::TSimCallbackObject<
	FPhysicsReplicationAsyncInput,
	Chaos::FSimCallbackNoOutput,
	Chaos::ESimCallbackOptions::Presimulate>
{
	virtual FName GetFNameForStatId() const override;
	virtual void OnPreSimulate_Internal() override;
	virtual void ApplyTargetStatesAsync(const float DeltaSeconds, const FPhysicsRepErrorCorrectionData& ErrorCorrection, const TArray<FPhysicsRepAsyncInputData>& TargetStates);

	// Replication functions
	virtual void DefaultReplication_DEPRECATED(Chaos::FRigidBodyHandle_Internal* Handle, const FPhysicsRepAsyncInputData& State, const float DeltaSeconds, const FPhysicsRepErrorCorrectionData& ErrorCorrection);
	virtual bool DefaultReplication(Chaos::FPBDRigidParticleHandle* Handle, FReplicatedPhysicsTargetAsync& Target, const float DeltaSeconds);
	virtual bool PredictiveInterpolation(Chaos::FPBDRigidParticleHandle* Handle, FReplicatedPhysicsTargetAsync& Target, const float DeltaSeconds);
	virtual bool ResimulationReplication(Chaos::FPBDRigidParticleHandle* Handle, FReplicatedPhysicsTargetAsync& Target, const float DeltaSeconds);

private:
	float LatencyOneWay;
	FRigidBodyErrorCorrection ErrorCorrectionDefault;
	TMap<Chaos::FPhysicsObject*, FReplicatedPhysicsTargetAsync> ObjectToTarget;

private:
	void UpdateAsyncTarget(const FPhysicsRepAsyncInputData& Input);
	void UpdateRewindDataTarget(const FPhysicsRepAsyncInputData& Input);

public:
	void Setup(FRigidBodyErrorCorrection ErrorCorrection)
	{
		ErrorCorrectionDefault = ErrorCorrection;
	}
};



class FPhysicsReplicationVR : public FPhysicsReplication
{
public:

	FPhysScene* PhysSceneVR;

	FPhysicsReplicationVR(FPhysScene* PhysScene);
	~FPhysicsReplicationVR();
	static bool IsInitialized();

	virtual void OnTick(float DeltaSeconds, TMap<TWeakObjectPtr<UPrimitiveComponent>, FReplicatedPhysicsTarget>& ComponentsToTargets) override;
	
	virtual bool ApplyRigidBodyState(float DeltaSeconds, FBodyInstance* BI, FReplicatedPhysicsTarget& PhysicsTarget, const FRigidBodyErrorCorrection& ErrorCorrection, const float PingSecondsOneWay, int32 LocalFrame, int32 NumPredictedFrames) override;
	virtual bool ApplyRigidBodyState(float DeltaSeconds, FBodyInstance* BI, FReplicatedPhysicsTarget& PhysicsTarget, const FRigidBodyErrorCorrection& ErrorCorrection, const float PingSecondsOneWay, bool* bDidHardSnap = nullptr) override;

	virtual void SetReplicatedTarget(UPrimitiveComponent* Component, FName BoneName, const FRigidBodyState& ReplicatedTarget, int32 ServerFrame) override;
	void SetReplicatedTargetVR(Chaos::FPhysicsObject* PhysicsObject, const FRigidBodyState& ReplicatedTarget, int32 ServerFrame, EPhysicsReplicationMode ReplicationMode);
	
	TArray<FReplicatedPhysicsTarget> ReplicatedTargetsQueueVR;
	FPhysicsReplicationAsyncVR* PhysicsReplicationAsyncVR;
	FPhysicsReplicationAsyncInput* AsyncInputVR;	//async data being written into before we push into callback

	void PrepareAsyncData_ExternalVR(const FRigidBodyErrorCorrection& ErrorCorrection);	//prepare async data for writing. Call on external thread (i.e. game thread)
};

class IPhysicsReplicationFactoryVR : public IPhysicsReplicationFactory
{
public:

	virtual TUniquePtr<IPhysicsReplication> CreatePhysicsReplication(FPhysScene* OwningPhysScene) override
	{
		return TUniquePtr<IPhysicsReplication>(new FPhysicsReplicationVR(OwningPhysScene));
	}

	/*virtual FPhysicsReplication* Create(FPhysScene* OwningPhysScene)
	{
		return new FPhysicsReplicationVR(OwningPhysScene);
	}

	virtual void Destroy(FPhysicsReplication* PhysicsReplication)
	{
		if (PhysicsReplication)
			delete PhysicsReplication;
	}*/
};

//#endif

USTRUCT()
struct VREXPANSIONPLUGIN_API FRepMovementVR : public FRepMovement
{
	GENERATED_USTRUCT_BODY()
public:

	FRepMovementVR();
	FRepMovementVR(FRepMovement& other);
	void CopyTo(FRepMovement& other) const;
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
	bool GatherActorsMovement(AActor* OwningActor);
};

template<>
struct TStructOpsTypeTraits<FRepMovementVR> : public TStructOpsTypeTraitsBase2<FRepMovementVR>
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};

USTRUCT(BlueprintType)
struct VREXPANSIONPLUGIN_API FVRClientAuthReplicationData
{
	GENERATED_BODY()
public:

	// If True and we are using a client auth grip type then we will replicate our throws on release
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRReplication")
		bool bUseClientAuthThrowing;

	// Rate that we will be sending throwing events to the server, not replicated, only serialized
	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadOnly, Category = "VRReplication", meta = (ClampMin = "0", UIMin = "0", ClampMax = "100", UIMax = "100"))
		int32 UpdateRate;

	FTimerHandle ResetReplicationHandle;
	FTransform LastActorTransform;
	float TimeAtInitialThrow;
	bool bIsCurrentlyClientAuth;

	FVRClientAuthReplicationData() :
		bUseClientAuthThrowing(false),
		UpdateRate(30),
		LastActorTransform(FTransform::Identity),
		TimeAtInitialThrow(0.0f),
		bIsCurrentlyClientAuth(false)
	{

	}
};