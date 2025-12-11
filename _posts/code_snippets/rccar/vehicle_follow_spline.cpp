void UBTTask_VehicleFollowSpline::CalculateSplineGoal(UBehaviorTreeComponent& OwnerComp, float DeltaSeconds)
{
    Time += DeltaSeconds;

    // Cache current vehicle state
    FVector ActorLocation = SelfActor->GetActorLocation();
    FVector ActorForwardVector = SelfActor->GetActorForwardVector();
    float SpeedKMH = VehicleMovement->GetSpeedKMH();
    
    // Calculate current position along spline
    float DistanceAlongSpline = Spline->GetDistanceAlongSplineAtSplineInputKey(CurrentGoalKey) - DistanceThreshold;
    FVector VehicleOnSpline = Spline->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
    
    VehicleAI->DistanceAlongSpline = DistanceAlongSpline;

    // === Dynamic Lane Positioning ===
    // Creates a sine wave pattern to vary the racing line
    FVector RightVector = Spline->GetRightVectorAtSplineInputKey(CurrentGoalKey, ESplineCoordinateSpace::World);
    OffsetTime += DeltaSeconds / 4;
    float OffsetSine = FMath::Sin(OffsetTime);
    FVector OffsetVector = RightVector * (100 * OffsetSine);

    Goal = Spline->GetLocationAtSplineInputKey(CurrentGoalKey, ESplineCoordinateSpace::World);
    Goal += OffsetVector;

    // === Adaptive Goal Progression ===
    // Adjusts how far ahead the AI looks based on speed
    FVector2D SpeedRange = FVector2D(10, 60);
    FVector2D GoalDistanceRange = FVector2D(100, 1000);
    
    if(FVector::Distance(Goal, ActorLocation) < DistanceThreshold)
    {
        // Map current speed to appropriate goal distance
        float TargetDistance = FMath::GetMappedRangeValueClamped(SpeedRange, GoalDistanceRange, SpeedKMH);
        
        if(Time < 5)
        {
            // Accelerate distance threshold during startup
            DistanceThreshold = FMath::Max(1000 - (Time * 150), TargetDistance);
        }
        else
        {
            // Smooth interpolation to target distance
            DistanceThreshold = FMath::FInterpTo(DistanceThreshold, TargetDistance, DeltaSeconds, 8);
        }

        // Progress along spline based on speed
        float SplinePoints = Spline->GetNumberOfSplinePoints();
        CurrentGoalKey += 2 * DeltaSeconds + SpeedKMH / 1200;
        CurrentGoalKey = FMath::Fmod(CurrentGoalKey, SplinePoints);
    }

    // === Stuck Detection ===
    if (!bIsStuck) 
    {
        if (SpeedKMH > -1 && SpeedKMH < 1)
        {
            TimeStuck += DeltaSeconds;
            if (TimeStuck >= 1.5) 
            {
                bIsStuck = true;
                TimeStuck = 0;
            }
        }
        else
        {
            TimeStuck = 0;
        }
    }
    else
    {
        // Check if path ahead is clear for recovery
        FHitResult Hit;
        FCollisionQueryParams TraceParams(FName(TEXT("StuckCheck")), true, SelfActor);
        TraceParams.bTraceComplex = true;
        
        FVector StartRay = ActorLocation + FVector(0, 0, 20);
        FVector EndRay = StartRay + (ActorForwardVector * 150);

        bool bIsHit = GetWorld()->LineTraceSingleByChannel(Hit, StartRay, EndRay, 
            ECollisionChannel::ECC_WorldStatic, TraceParams);
        
        if (!bIsHit)
        {
            // Path is clear, exit stuck state and proceed
            float SplinePoints = Spline->GetNumberOfSplinePoints();
            Goal = Spline->GetLocationAtSplineInputKey(FMath::Fmod(CurrentGoalKey + 1, SplinePoints), 
                ESplineCoordinateSpace::World);
            Goal += OffsetVector;
            
            OwnerComp.GetBlackboardComponent()->SetValueAsVector(WaypointKey.SelectedKeyName, Goal);
            FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
            return;
        }
    }

    // === Steering Calculation ===
    FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(ActorLocation, Goal);
    FRotator SteeringDelta = UKismetMathLibrary::NormalizedDeltaRotator(TargetRotation, SelfActor->GetActorRotation());
    float SteeringValue = SteeringDelta.Yaw / 90.0;

    if (!bIsStuck) 
    {
        VehicleMovement->SetVehicleSteering(SteeringValue);
    }
    else 
    {
        VehicleMovement->SetVehicleSteering(0);
    }

    // === Throttle Control ===
    float DistanceToSpline = FVector::Distance(VehicleOnSpline + OffsetVector, ActorLocation);
    float MaxThrottle = VehicleMovement->MaxThrottleBySplineDistance.GetRichCurve()->Eval(DistanceToSpline);

    // === Dynamic Braking System ===
    // Look ahead to anticipate upcoming curves
    FVector2D LookaheadRange = FVector2D(0.1, 0.5);
    float LookaheadDistance = FMath::GetMappedRangeValueClamped(SpeedRange, LookaheadRange, SpeedKMH);
    
    float SplinePoints = Spline->GetNumberOfSplinePoints();
    FVector UpcomingDirection = Spline->GetDirectionAtSplineInputKey(
        FMath::Fmod(CurrentGoalKey + LookaheadDistance, SplinePoints), 
        ESplineCoordinateSpace::World);
    
    // Calculate how aligned we are with the upcoming path
    float DotProduct = FVector::DotProduct(ActorForwardVector, UpcomingDirection);
    
    // Determine maximum safe speed for upcoming curve
    float MaxSpeed = CalculateMaxSpeedByDriveStyle(
        VehicleMovement->MaxSpeedBySplineForwardFactor.GetRichCurve()->Eval(DotProduct));
    
    // Apply progressive braking if going too fast
    float ExcessSpeed = SpeedKMH - MaxSpeed;
    float BrakeInput = FMath::Clamp(ExcessSpeed / 40, 0.0, 1.0);

    if(bIsStuck)
    {
        VehicleMovement->SetVehicleThrottleAndBrake(0, 0.5);
    }
    else
    {
        VehicleMovement->SetVehicleThrottleAndBrake(MaxThrottle, BrakeInput);
    }
}

float UBTTask_VehicleFollowSpline::CalculateMaxSpeedByDriveStyle(float MaxSpeed)
{
    // Allows for different AI driving personalities (aggressive, cautious, etc.)
    return MaxSpeed + VehicleAI->MaxSpeedDelta;
}