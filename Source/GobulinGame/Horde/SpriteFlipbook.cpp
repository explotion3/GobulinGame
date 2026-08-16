#include "Horde/SpriteFlipbook.h"

FVector2D USpriteFlipbook::GetFrameSize() const
{
	const int32 SafeColumns = FMath::Max(1, Columns);
	const int32 SafeRows = FMath::Max(1, Rows);
	return FVector2D(1.0f / SafeColumns, 1.0f / SafeRows);
}

FVector2D USpriteFlipbook::GetFrameOffset(int32 DirectionIndex, int32 FrameIndex) const
{
	const int32 SafeColumns = FMath::Max(1, Columns);
	const int32 SafeRows = FMath::Max(1, Rows);
	const int32 SafeDirections = FMath::Max(1, DirectionCount);
	const int32 ClampedDirection = FMath::Clamp(DirectionIndex, 0, SafeDirections - 1);

	int32 RowIndex = 0;
	if (bDirectionsUseSeparateRows)
	{
		const int32 RowsPerDirection = FMath::Max(1, SafeRows / SafeDirections);
		RowIndex = ClampedDirection * RowsPerDirection + (FrameIndex / SafeColumns);
	}
	else
	{
		RowIndex = FrameIndex / SafeColumns;
	}

	const int32 ColIndex = FrameIndex % SafeColumns;
	return FVector2D(ColIndex * (1.0f / SafeColumns), RowIndex * (1.0f / SafeRows));
}

const FSpriteClip* USpriteFlipbook::GetClip(FName ClipId) const
{
	return Clips.FindByPredicate([ClipId](const FSpriteClip& Clip)
	{
		return Clip.ClipId == ClipId;
	});
}
