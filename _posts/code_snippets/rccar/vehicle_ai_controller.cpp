void AVehicleAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!VehiclePawn)
        return;

    if(bUseAvoidance && TargetActor)
    {
        FVector SourceLocation = VehiclePawn->GetActorLocation();
        FVector TargetLocation = TargetActor->GetActorLocation();

        // === Distance Check ===
        // Remove target if it's too far away to matter
        float Distance = FVector::Distance(SourceLocation, TargetLocation);
        if(Distance > 400)
        {
            SetAIPerceptionTarget(nullptr);
            PerceivedActors.Remove(TargetActor);
            return;
        }

        // === Calculate Avoidance Direction ===
        FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(SourceLocation, TargetLocation);
        FRotator TargetRotation = TargetActor->GetActorRotation();
        float DeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(LookAtRotation, TargetRotation).Yaw;
        float SteeringValue = UKismetMathLibrary::SignOfFloat(DeltaYaw) * -1.0;
        float DeltaYawAbs = FMath::Abs(DeltaYaw);

        // === Dynamic Steering Weight ===
        // InRange: Distance between source and target
        // OutRange: Weight based on distance
        // Logic: Closer to target requires stronger steering adjustments
        float SteeringModifierDistanceWeight = UKismetMathLibrary::MapRangeClamped(
            Distance, 0.0, MaxAvoidanceDistance, MaxAvoidanceWeight, 0.0);

        // === Angle-Based Steering Adjustment ===
        // InRange: Delta of rotation towards target
        // 0°: Target directly ahead - maximum steering needed
        // 180°: Target is behind - no steering needed
        float SteeringModifierWeight = UKismetMathLibrary::MapRangeClamped(
            DeltaYawAbs, 0.0, MaxAvoidanceAngleDelta, SteeringModifierDistanceWeight, 0.0);

        SourceMovementComponent->SetVehicleSteeringModifier(SteeringValue, SteeringModifierWeight);

        // === Proximity Braking ===
        // Apply brakes if vehicle is approaching too quickly
        if(SourceMovementComponent->GetSpeedKMH() > MinBrakingSpeed)
        {
            // InRange: Distance between source and target
            // OutRange: Weight based on distance
            // Logic: Closer to target requires more aggressive braking
            float BrakingModifierWeight = UKismetMathLibrary::MapRangeClamped(
                Distance, 0.0, MaxBrakingDistance, MaxBrakingWeight, 0.0);

            SourceMovementComponent->SetVehicleBrakingModifier(1.0, BrakingModifierWeight);
        }
    }
}

void AVehicleAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
    PerceivedActors = UpdatedActors;

    if (!bUseAvoidance || !VehiclePawn)
        return;

    // === No Targets Detected ===
    if (PerceivedActors.Num() == 0) 
    {
        SetAIPerceptionTarget(nullptr);
        return;
    }

    // === Single Target ===
    if (PerceivedActors.Num() == 1) 
    {
        SetAIPerceptionTarget(PerceivedActors[0]);
        return;
    }

    // === Multiple Targets - Find Closest ===
    AActor* ClosestTarget = nullptr;
    float ClosestDistance = 100000;
    
    for (AActor* PerceivedActor : PerceivedActors)
    {
        if (ClosestTarget == nullptr)
        {
            ClosestTarget = PerceivedActor;
            continue;
        }

        float Distance = FVector::Distance(PerceivedActor->GetActorLocation(), VehiclePawn->GetActorLocation());
        if(Distance < ClosestDistance)
        {
            ClosestDistance = Distance;
            ClosestTarget = PerceivedActor;
        }
    }

    if(ClosestTarget)
    {
        SetAIPerceptionTarget(ClosestTarget);
    }
}

void AVehicleAIController::SetAIPerceptionTarget(AActor* Target)
{
    // Avoid mistaking yourself as a perception target
    if(Target && VehiclePawn != Target)
    {
        if(TargetActor != Target)
        {
            if(ARCVehiclePawn* TargetPawn = Cast<ARCVehiclePawn>(Target))
            {
                TargetActor = Target;
                TargetMovementComponent = TargetPawn->GetRCVehicleMovementComponent();
            }
        }
    }
    else 
    {
        // Clear target and reset all modifiers
        if (TargetActor)
        {
            TargetActor = nullptr;
            TargetMovementComponent = nullptr;
            SourceMovementComponent->SetVehicleSteeringModifier(0, 0);
            SourceMovementComponent->SetVehicleThrottleModifier(0, 0);
            SourceMovementComponent->SetVehicleBrakingModifier(0, 0);
        }
    }
}