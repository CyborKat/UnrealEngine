// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorDockableWindow.h"
#include "VREditorUISystem.h"
#include "VREditorMode.h"
#include "VREditorBaseUserWidget.h"
#include "WidgetComponent.h"
#include "VREditorWidgetComponent.h"

namespace VREd
{
	static FAutoConsoleVariable DockWindowThickness( TEXT( "VREd.DockWindowTickness" ), 1.0f, TEXT( "How thick the window is" ) );
	static FAutoConsoleVariable DockUISelectionBarVerticalOffset( TEXT( "VREd.DockUISelectionBarVerticalOffset" ), 2.0f, TEXT( "Z Distance between the selectionbar and the UI" ) );
	static FAutoConsoleVariable DockUICloseButtonOffsetFromCorner( TEXT( "VREd.DockUICloseButtonOffsetFromCorner" ), 1.5f, TEXT( "How far away from the corner (along each 2D axis) of the UI the close button draws" ) );
	static FAutoConsoleVariable DockUIFadeAnimationDuration( TEXT( "VREd.DockUIFadeAnimationDuration" ), 0.15f, TEXT( "How quick the fade animation should complete in" ) );
	static FAutoConsoleVariable DockUIHoverScale( TEXT( "VREd.DockUIHoverScale" ), 1.15f, TEXT( "How big the selection bar gets when you hover over it" ) );
	static FAutoConsoleVariable DockUIHoverAnimationDuration( TEXT( "VREd.DockUIHoverAnimationDuration" ), 0.15f, TEXT( "How quick the hover animation should complete in" ) );
}

AVREditorDockableWindow::AVREditorDockableWindow() 
	: Super()
{

	{
		UStaticMesh* WindowMesh = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/UI/SM_ContentWindow_01" ) );
			WindowMesh = ObjectFinder.Object;
			check( WindowMesh != nullptr );
		}

		WindowMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "WindowMesh" ) );
		WindowMeshComponent->SetStaticMesh( WindowMesh );
		WindowMeshComponent->SetMobility( EComponentMobility::Movable );
		WindowMeshComponent->AttachTo( RootComponent );

		WindowMeshComponent->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
		WindowMeshComponent->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Ignore );
		WindowMeshComponent->SetCollisionResponseToChannel( ECC_EditorGizmo, ECollisionResponse::ECR_Block );
		WindowMeshComponent->SetCollisionObjectType( ECC_EditorGizmo );

		WindowMeshComponent->bGenerateOverlapEvents = false;
		WindowMeshComponent->SetCanEverAffectNavigation( false );
		WindowMeshComponent->bCastDynamicShadow = false;
		WindowMeshComponent->bCastStaticShadow = false;
		WindowMeshComponent->bAffectDistanceFieldLighting = false;
	}

	{
		UStaticMesh* SelectionMesh = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/UI/SelectionBarMesh" ) );
			SelectionMesh = ObjectFinder.Object;
			check( SelectionMesh != nullptr );
		}

		SelectionBarMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "SelectionBarMesh" ) );
		SelectionBarMeshComponent->SetStaticMesh( SelectionMesh );
		SelectionBarMeshComponent->SetMobility( EComponentMobility::Movable );
		SelectionBarMeshComponent->AttachTo( RootComponent );

		SelectionBarMeshComponent->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
		SelectionBarMeshComponent->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Ignore );
		SelectionBarMeshComponent->SetCollisionResponseToChannel( ECC_EditorGizmo, ECollisionResponse::ECR_Block );
		SelectionBarMeshComponent->SetCollisionObjectType( ECC_EditorGizmo );

		SelectionBarMeshComponent->bGenerateOverlapEvents = false;
		SelectionBarMeshComponent->SetCanEverAffectNavigation( false );
		SelectionBarMeshComponent->bCastDynamicShadow = false;
		SelectionBarMeshComponent->bCastStaticShadow = false;
		SelectionBarMeshComponent->bAffectDistanceFieldLighting = false;

		UMaterial* HoverMaterial = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UMaterial> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/TransformGizmoMaterial" ) );
			HoverMaterial = ObjectFinder.Object;
			check( HoverMaterial != nullptr );
		}
		SelectionBarMID = UMaterialInstanceDynamic::Create( HoverMaterial, GetTransientPackage() );
		check( SelectionBarMID != nullptr );
		SelectionBarMeshComponent->SetMaterial( 0, SelectionBarMID );

		UMaterial* TranslucentHoverMaterial = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UMaterial> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/TranslucentTransformGizmoMaterial" ) );
			TranslucentHoverMaterial = ObjectFinder.Object;
			check( TranslucentHoverMaterial != nullptr );
		}
		SelectionBarTranslucentMID = UMaterialInstanceDynamic::Create( TranslucentHoverMaterial, GetTransientPackage() );
		check( SelectionBarTranslucentMID != nullptr );
		SelectionBarMeshComponent->SetMaterial( 1, SelectionBarTranslucentMID );
	}

	{
		UStaticMesh* CloseButtonMesh = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UStaticMesh> ObjectFinder( TEXT( "/Engine/VREditor/UI/CloseButtonMesh" ) );
			CloseButtonMesh = ObjectFinder.Object;
			check( CloseButtonMesh != nullptr );
		}

		CloseButtonMeshComponent= CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "CloseButtonMesh" ) );
		CloseButtonMeshComponent->SetStaticMesh( CloseButtonMesh );
		CloseButtonMeshComponent->SetMobility( EComponentMobility::Movable );
		CloseButtonMeshComponent->AttachTo( RootComponent );

		CloseButtonMeshComponent->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
		CloseButtonMeshComponent->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Ignore );
		CloseButtonMeshComponent->SetCollisionResponseToChannel( ECC_EditorGizmo, ECollisionResponse::ECR_Block );
		CloseButtonMeshComponent->SetCollisionObjectType( ECC_EditorGizmo );

		CloseButtonMeshComponent->bGenerateOverlapEvents = false;
		CloseButtonMeshComponent->SetCanEverAffectNavigation( false );
		CloseButtonMeshComponent->bCastDynamicShadow = false;
		CloseButtonMeshComponent->bCastStaticShadow = false;
		CloseButtonMeshComponent->bAffectDistanceFieldLighting = false;

		UMaterial* HoverMaterial = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UMaterial> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/TransformGizmoMaterial" ) );
			HoverMaterial = ObjectFinder.Object;
			check( HoverMaterial != nullptr );
		}
		CloseButtonMID = UMaterialInstanceDynamic::Create( HoverMaterial, GetTransientPackage() );
		check( CloseButtonMID != nullptr );
		CloseButtonMeshComponent->SetMaterial( 0, CloseButtonMID );

		UMaterial* TranslucentHoverMaterial = nullptr;
		{
			static ConstructorHelpers::FObjectFinder<UMaterial> ObjectFinder( TEXT( "/Engine/VREditor/TransformGizmo/TranslucentTransformGizmoMaterial" ) );
			TranslucentHoverMaterial = ObjectFinder.Object;
			check( TranslucentHoverMaterial != nullptr );
		}
		CloseButtonTranslucentMID = UMaterialInstanceDynamic::Create( TranslucentHoverMaterial, GetTransientPackage() );
		check( CloseButtonTranslucentMID != nullptr );
		CloseButtonMeshComponent->SetMaterial( 1, CloseButtonTranslucentMID );
	}

	// The selection bar and close button will not be initially visible.  They'll appear when the user aims
	// their laser toward the UI
	SelectionBarMeshComponent->SetVisibility( false );
	CloseButtonMeshComponent->SetVisibility( false );
}


void AVREditorDockableWindow::SetupWidgetComponent()
{
	Super::SetupWidgetComponent();

	SetSelectionBarColor( GetOwner().GetOwner().GetColor( FVREditorMode::EColors::UISelectionBarColor ) );
	SetCloseButtonColor( GetOwner().GetOwner().GetColor( FVREditorMode::EColors::UICloseButtonColor ) );
}


void AVREditorDockableWindow::TickManually( float DeltaTime )
{
	Super::TickManually( DeltaTime );

	if( WidgetComponent->IsVisible() )
	{
		const float WorldScaleFactor = GetOwner().GetOwner().GetWorldScaleFactor();
		const FVector AnimatedScale = CalculateAnimatedScale();


		// Update whether the user is aiming toward us or not
		bIsLaserAimingTowardUI = false;
		{
			const FTransform UICapsuleTransform = this->GetActorTransform();

			const FVector UICapsuleStart = FVector( 0.0f, 0.0f, -GetSize().Y * 0.4f ) * WorldScaleFactor * AnimatedScale;
			const FVector UICapsuleEnd = FVector( 0.0f, 0.0f, GetSize().Y * 0.4f ) * WorldScaleFactor * AnimatedScale;
			const float UICapsuleLocalRadius = GetSize().X * 0.5f * WorldScaleFactor * AnimatedScale.Y;
			const float MinDistanceToUICapsule = 10.0f * WorldScaleFactor * AnimatedScale.Y;	// @todo vreditor tweak
			const FVector UIForwardVector = FVector::ForwardVector;
			const float MinDotForAimingAtOtherHand = -1.1f;	// @todo vreditor tweak

			for( int32 HandIndex = 0; HandIndex < VREditorConstants::NumVirtualHands; ++HandIndex )
			{
				if( GetOwner().GetOwner().IsHandAimingTowardsCapsule( HandIndex, UICapsuleTransform, UICapsuleStart, UICapsuleEnd, UICapsuleLocalRadius, MinDistanceToUICapsule, UIForwardVector, MinDotForAimingAtOtherHand ) )
				{
					bIsLaserAimingTowardUI = true;
				}
			}

			if( bIsLaserAimingTowardUI )
			{
				AimingAtMeFadeAlpha += DeltaTime / VREd::DockUIFadeAnimationDuration->GetFloat();
			}
			else
			{
				AimingAtMeFadeAlpha -= DeltaTime / VREd::DockUIFadeAnimationDuration->GetFloat();;
			}
			AimingAtMeFadeAlpha = FMath::Clamp( AimingAtMeFadeAlpha, 0.0f, 1.0f );
		}

		const float AnimationOvershootAmount = 1.0f;	// @todo vreditor tweak
		float EasedAimingAtMeFadeAlpha = FVREditorMode::OvershootEaseOut( AimingAtMeFadeAlpha, AnimationOvershootAmount );

		// Only show our extra buttons and controls if the user is roughly aiming toward us.  This just reduces clutter.
		SelectionBarMeshComponent->SetVisibility( EasedAimingAtMeFadeAlpha > KINDA_SMALL_NUMBER ? true : false );
		CloseButtonMeshComponent->SetVisibility( EasedAimingAtMeFadeAlpha > KINDA_SMALL_NUMBER ? true : false );

		EasedAimingAtMeFadeAlpha = FMath::Max( 0.001f, EasedAimingAtMeFadeAlpha );

		// Update the window border mesh
		{
			const float WindowMeshSize = 100.0f;	// Size of imported mesh, we need to inverse compensate for

			const FVector WindowMeshScale = FVector(
				VREd::DockWindowThickness->GetFloat(),
				GetSize().X / WindowMeshSize,
				GetSize().Y / WindowMeshSize ) * AnimatedScale * WorldScaleFactor;
			WindowMeshComponent->SetRelativeScale3D( WindowMeshScale );			
		}

		// Update the selection bar
		{
			if( bIsHoveringOverSelectionBar )
			{
				SelectionBarHoverAlpha += DeltaTime / VREd::DockUIHoverAnimationDuration->GetFloat();
			}
			else
			{
				SelectionBarHoverAlpha -= DeltaTime / VREd::DockUIHoverAnimationDuration->GetFloat();;
			}
			SelectionBarHoverAlpha = FMath::Clamp( SelectionBarHoverAlpha, 0.0f, 1.0f );

			// How big the selection bar should be
			const FVector SelectionBarSize( 1.0f, GetSize().X * 0.8f, 6.0f );	// @todo vreditor tweak
			FVector SelectionBarScale = SelectionBarSize * AnimatedScale * WorldScaleFactor;
			SelectionBarScale *= FMath::Lerp( 1.0f, VREd::DockUIHoverScale->GetFloat(), SelectionBarHoverAlpha );

			// Scale vertically based on our fade alpha
			SelectionBarScale.Z *= EasedAimingAtMeFadeAlpha;

			SelectionBarMeshComponent->SetRelativeScale3D( SelectionBarScale );
			const FVector SelectionBarRelativeLocation = FVector(
				0.0f,
				0.0f,
				-( GetSize().Y * 0.5f + SelectionBarSize.Z * 0.5f + VREd::DockUISelectionBarVerticalOffset->GetFloat() ) ) * AnimatedScale * WorldScaleFactor;
			SelectionBarMeshComponent->SetRelativeLocation( SelectionBarRelativeLocation );

			SetSelectionBarColor( GetOwner().GetOwner().GetColor( bIsHoveringOverSelectionBar ? FVREditorMode::EColors::UISelectionBarHoverColor : FVREditorMode::EColors::UISelectionBarColor ) );
		}

		// Update the close button
		{
			if( bIsHoveringOverCloseButton )
			{
				CloseButtonHoverAlpha += DeltaTime / VREd::DockUIHoverAnimationDuration->GetFloat();
			}
			else
			{
				CloseButtonHoverAlpha -= DeltaTime / VREd::DockUIHoverAnimationDuration->GetFloat();;
			}
			CloseButtonHoverAlpha = FMath::Clamp( CloseButtonHoverAlpha, 0.0f, 1.0f );

			// How big the close button should be
			const FVector CloseButtonSize( 1.0f, 2.0f, 2.0f );	// @todo vreditor tweak
			FVector CloseButtonScale = CloseButtonSize * AnimatedScale * WorldScaleFactor * EasedAimingAtMeFadeAlpha;
			CloseButtonScale *= FMath::Lerp( 1.0f, VREd::DockUIHoverScale->GetFloat(), CloseButtonHoverAlpha );


			CloseButtonMeshComponent->SetRelativeScale3D( CloseButtonScale );
			const FVector CloseButtonRelativeLocation = FVector(
				0.0f,
				-( GetSize().X * 0.5f + CloseButtonSize.Y * 0.5f + VREd::DockUICloseButtonOffsetFromCorner->GetFloat() ),
				( GetSize().Y * 0.5f + CloseButtonSize.Z * 0.5f + VREd::DockUICloseButtonOffsetFromCorner->GetFloat() ) ) * AnimatedScale * WorldScaleFactor;
			CloseButtonMeshComponent->SetRelativeLocation( CloseButtonRelativeLocation );

			SetCloseButtonColor( GetOwner().GetOwner().GetColor( bIsHoveringOverCloseButton ? FVREditorMode::EColors::UICloseButtonHoverColor : FVREditorMode::EColors::UICloseButtonColor ) );
		}
	}
} 

void AVREditorDockableWindow::UpdateRelativeRoomTransform()
{
	const FTransform RoomToWorld = GetOwner().GetOwner().GetRoomTransform();
	const FTransform WorldToRoom = RoomToWorld.Inverse();

	const FTransform WindowToWorldTransform = GetActorTransform();
	const FTransform WindowToRoomTransform = WindowToWorldTransform * WorldToRoom;

	const FVector RoomSpaceWindowLocation = WindowToRoomTransform.GetLocation() / GetOwner().GetOwner().GetWorldScaleFactor();;
	const FQuat RoomSpaceWindowRotation = WindowToRoomTransform.GetRotation();

	RelativeOffset = RoomSpaceWindowLocation;
	LocalRotation = RoomSpaceWindowRotation.Rotator();
}

void AVREditorDockableWindow::OnEnterHover( const FHitResult& HitResult, const int32 HandIndex )
{
	if( SelectionBarMeshComponent == HitResult.GetComponent() )
	{
		bIsHoveringOverSelectionBar = true;
	}
	
	if( CloseButtonMeshComponent == HitResult.GetComponent() )
	{
		bIsHoveringOverCloseButton = true;
	}
}

void AVREditorDockableWindow::OnLeaveHover( const int32 HandIndex, const UActorComponent* NewComponent  )
{
	FVirtualHand& OtherHand = GetOwner().GetOwner().GetOtherHand( HandIndex );

	if( OtherHand.HoveredActorComponent != SelectionBarMeshComponent && NewComponent != SelectionBarMeshComponent )
	{
		bIsHoveringOverSelectionBar = false;
	}
	
	if( OtherHand.HoveredActorComponent != CloseButtonMeshComponent && NewComponent != CloseButtonMeshComponent  )
	{
		bIsHoveringOverCloseButton = false;
	}
}


void AVREditorDockableWindow::SetSelectionBarColor( const FLinearColor& LinearColor )
{
	static FName StaticColorParameterName( "Color" );
	SelectionBarMID->SetVectorParameterValue( StaticColorParameterName, LinearColor );
	SelectionBarTranslucentMID->SetVectorParameterValue( StaticColorParameterName, LinearColor );
}


void AVREditorDockableWindow::SetCloseButtonColor( const FLinearColor& LinearColor )
{
	static FName StaticColorParameterName( "Color" );
	CloseButtonMID->SetVectorParameterValue( StaticColorParameterName, LinearColor );
	CloseButtonTranslucentMID->SetVectorParameterValue( StaticColorParameterName, LinearColor );
}
