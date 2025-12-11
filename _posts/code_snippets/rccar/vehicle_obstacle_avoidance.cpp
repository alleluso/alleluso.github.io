void UBTTask_VehicleObstacleAvoidance::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

    if (VehicleAI)
    {
        float Distance = 0;
        bool GoForward = false;
        bool bHasHit = false;

        FVector ActorLocation = SelfActor->GetActorLocation();
        FVector StartRay = ActorLocation + FVector(0, 0, 20);

        // === 360-Degree Obstacle Detection ===
        // Cast rays in a circle around the vehicle to find the nearest obstacle
        for (int i = 0; i < RaycastCount; i++)
        {
            float Angle = (360.0 / RaycastCount) * (float)i;
            FVector Direction = SelfActor->GetActorRightVector().RotateAngleAxis(Angle, SelfActor->GetActorUpVector());

            FHitResult Hit;
            FCollisionQueryParams TraceParams = FCollisionQueryParams(FName(TEXT("ObstacleCheck")), true, SelfActor);
            TraceParams.bTraceComplex = true;
            TraceParams.bReturnPhysicalMaterial = false;
            
            FVector EndRay = ActorLocation + (Direction * RaycastDistance);
            bool bIsHit = GetWorld()->LineTraceSingleByChannel(Hit, StartRay, EndRay, TRACE_VehicleAvoidance, TraceParams);
            
            if (bIsHit)
            {
                // Track the closest obstacle
                if (!bHasHit) 
                {
                    bHasHit = true;
                    RaycastLocation = Hit.ImpactPoint;
                    Distance = Hit.Distance;
                    GoForward = Angle < 180;  // Determine if we should move forward or backward
                    HitAngle = Angle;
                    continue;
                }

                if (Hit.Distance < Distance) 
                {
                    RaycastLocation = Hit.ImpactPoint;
                    Distance = Hit.Distance;
                    GoForward = Angle < 180;
                    HitAngle = Angle;
                }
            }
        }

        // === Path Clear - Exit Avoidance Mode ===
        if (!bHasHit) 
        {
            TimeWithoutHit += DeltaSeconds;
            HitTime = 0;

            // Wait for a brief period to ensure path is clear before exiting
            if(TimeWithoutHit >= 0.5 || FVector::Dist(ActorLocation, RaycastLocation) > RaycastDistance)
            {
                VehicleMovement->SetVehicleSteering(0);
                VehicleMovement->SetVehicleThrottleAndBrake(0, 0);
                VehicleMovement->SetVehicleHandbrake(true);
                FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
            }

            return;
        }
        else 
        {
            TimeWithoutHit = 0;
            HitTime += DeltaSeconds;

            // === Calculate Avoidance Direction ===
            // Compute direction away from the obstacle
            FVector RaycastDirection = ((ActorLocation - RaycastLocation) * 
                (GoForward ? FVector(1, 1, 0) : FVector(-1, -1, 0))).GetSafeNormal();
            FVector ActorDirection = (SelfActor->GetActorForwardVector() * FVector(1, 1, 0)).GetSafeNormal();
            
            // Blend between current direction and avoidance direction based on alignment
            float Dot = FVector::DotProduct(RaycastDirection, ActorDirection);
            FVector GoalLocation = FMath::Lerp(ActorDirection, RaycastDirection, FMath::Abs(Dot)) * 100 + ActorLocation;

            // === Steering Calculation ===
            FRotator Rotation = UKismetMathLibrary::FindLookAtRotation(ActorLocation, GoalLocation);
            FRotator SteeringDelta = UKismetMathLibrary::NormalizedDeltaRotator(Rotation, SelfActor->GetActorRotation());
            float SteeringValue = SteeringDelta.Yaw / 60.0;

            // === Vehicle Control ===
            // Choose between forward avoidance or reversing based on obstacle position
            if(GoForward)
            {
                VehicleMovement->SetVehicleSteering(SteeringValue);
                VehicleMovement->SetVehicleThrottleAndBrake(0.8, 0.0);
            }
            else 
            {
                VehicleMovement->SetVehicleSteering(-1 * SteeringValue);
                VehicleMovement->SetVehicleThrottleAndBrake(0, 0.8);
            }

            VehicleMovement->SetVehicleHandbrake(false);

            FinishLatentTask(OwnerComp, EBTNodeResult::InProgress);
            return;
        }
    }

    FinishLatentAbort(OwnerComp);
}