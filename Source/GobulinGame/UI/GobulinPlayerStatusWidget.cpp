#include "UI/GobulinPlayerStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> UGobulinPlayerStatusWidget::RebuildWidget()
{
	// 蓝图子类有自己的设计器内容时，保留其 WidgetTree；数据与反馈仍由下方蓝图事件输入。
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	TSharedRef<SOverlay> RootOverlay = SNew(SOverlay);

	RootOverlay->AddSlot()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Bottom)
	.Padding(FMargin(28.0f, 28.0f, 28.0f, 34.0f))
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.025f, 0.86f))
		.Padding(FMargin(16.0f, 12.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 7.0f))
			[
				SAssignNew(HealthText, STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
				.ColorAndOpacity(FLinearColor::White)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.WidthOverride(260.0f)
				.HeightOverride(18.0f)
				[
					SAssignNew(HealthProgressBar, SProgressBar)
				]
			]
		]
	];

	RootOverlay->AddSlot()
	[
		SAssignNew(DamageFlashBorder, SBorder)
		.Visibility(EVisibility::HitTestInvisible)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.75f, 0.015f, 0.01f, 0.0f))
	];

	RootOverlay->AddSlot()
	[
		SAssignNew(DeathOverlay, SBorder)
		.Visibility(bDeathVisible ? EVisibility::Visible : EVisibility::Collapsed)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.76f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 24.0f))
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("GobulinPlayerUI", "DeathTitle", "你已倒下"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 42))
				.ColorAndOpacity(FLinearColor(0.95f, 0.2f, 0.12f, 1.0f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SButton)
				.ContentPadding(FMargin(30.0f, 12.0f))
				.OnClicked(FOnClicked::CreateUObject(this, &UGobulinPlayerStatusWidget::HandleRestartClicked))
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("GobulinPlayerUI", "RestartButton", "重新开始"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
				]
			]
		]
	];

	UpdateNativeHealthVisuals();
	return RootOverlay;
}

void UGobulinPlayerStatusWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	HealthProgressBar.Reset();
	HealthText.Reset();
	DamageFlashBorder.Reset();
	DeathOverlay.Reset();
}

void UGobulinPlayerStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (DamageFlashRemaining <= 0.0f)
	{
		return;
	}

	DamageFlashRemaining = FMath::Max(0.0f, DamageFlashRemaining - InDeltaTime);
	const float FadeAlpha = DamageFlashDuration > KINDA_SMALL_NUMBER
		? DamageFlashRemaining / DamageFlashDuration
		: 0.0f;
	UpdateNativeDamageFlash(DamageFlashPeakOpacity * FadeAlpha * FadeAlpha);
}

void UGobulinPlayerStatusWidget::SetHealth(float CurrentHealth, float MaximumHealth)
{
	DisplayedMaximumHealth = FMath::Max(0.0f, MaximumHealth);
	DisplayedCurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, DisplayedMaximumHealth);

	const float HealthPercent = DisplayedMaximumHealth > KINDA_SMALL_NUMBER
		? DisplayedCurrentHealth / DisplayedMaximumHealth
		: 0.0f;
	UpdateNativeHealthVisuals();
	ReceiveHealthChanged(DisplayedCurrentHealth, DisplayedMaximumHealth, HealthPercent);
}

void UGobulinPlayerStatusWidget::PlayDamageFeedback(float AppliedDamage)
{
	if (!FMath::IsFinite(AppliedDamage) || AppliedDamage <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float DamageRatio = DisplayedMaximumHealth > KINDA_SMALL_NUMBER
		? FMath::Clamp(AppliedDamage / DisplayedMaximumHealth, 0.0f, 1.0f)
		: 1.0f;
	DamageFlashPeakOpacity = FMath::Lerp(
		MinimumDamageFlashOpacity,
		MaximumDamageFlashOpacity,
		FMath::Clamp(DamageRatio * 4.0f, 0.0f, 1.0f));
	DamageFlashRemaining = FMath::Max(0.0f, DamageFlashDuration);
	UpdateNativeDamageFlash(DamageFlashPeakOpacity);
	ReceiveDamageFeedback(AppliedDamage);
}

void UGobulinPlayerStatusWidget::ShowDeath()
{
	bDeathVisible = true;
	if (DeathOverlay)
	{
		DeathOverlay->SetVisibility(EVisibility::Visible);
	}
	ReceiveDeathShown();
}

void UGobulinPlayerStatusWidget::HideDeath()
{
	bDeathVisible = false;
	if (DeathOverlay)
	{
		DeathOverlay->SetVisibility(EVisibility::Collapsed);
	}
	ReceiveDeathHidden();
}

void UGobulinPlayerStatusWidget::RequestRestart()
{
	OnRestartRequested.Broadcast();
}

FReply UGobulinPlayerStatusWidget::HandleRestartClicked()
{
	RequestRestart();
	return FReply::Handled();
}

void UGobulinPlayerStatusWidget::UpdateNativeHealthVisuals()
{
	const float HealthPercent = DisplayedMaximumHealth > KINDA_SMALL_NUMBER
		? DisplayedCurrentHealth / DisplayedMaximumHealth
		: 0.0f;

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(HealthPercent);
		const FLinearColor FillColor = HealthPercent > 0.5f
			? FLinearColor(0.12f, 0.8f, 0.24f, 1.0f)
			: HealthPercent > 0.25f
				? FLinearColor(0.95f, 0.55f, 0.05f, 1.0f)
				: FLinearColor(0.9f, 0.08f, 0.04f, 1.0f);
		HealthProgressBar->SetFillColorAndOpacity(FillColor);
	}

	if (HealthText)
	{
		HealthText->SetText(FText::Format(
			NSLOCTEXT("GobulinPlayerUI", "HealthValue", "生命 {0} / {1}"),
			FText::AsNumber(FMath::CeilToInt(DisplayedCurrentHealth)),
			FText::AsNumber(FMath::CeilToInt(DisplayedMaximumHealth))));
	}
}

void UGobulinPlayerStatusWidget::UpdateNativeDamageFlash(float Opacity)
{
	if (DamageFlashBorder)
	{
		DamageFlashBorder->SetBorderBackgroundColor(
			FLinearColor(0.75f, 0.015f, 0.01f, FMath::Clamp(Opacity, 0.0f, 1.0f)));
	}
}
