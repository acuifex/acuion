#pragma once

class IVDebugOverlay
{
public:
	bool ScreenPosition(const Vector& vIn, Vector& vOut)
	{
		typedef bool (* oScreenPosition)(void*, const Vector&, Vector&);
		return getvfunc<oScreenPosition>(this, 11)(this, vIn, vOut);
	}
	void Drawbox(const Vector& origin, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b, int a, float flDuration)
	{
		typedef void(* OriginalFn)(void*, const Vector&, const Vector&, const QAngle&, int, int, int, int, float);
		return getvfunc<OriginalFn>(this, 22)(this, mins, maxs, angles, r, g, b, a, flDuration);
	}
	void DrawPill(const Vector& mins, const Vector& max, float& diameter, int r, int g, int b, int a, float duration)
	{
		typedef void(* OriginalFn)(void*, const Vector&, const Vector&, float&, int, int, int, int, float);
		return getvfunc<OriginalFn>(this, 24)(this, mins, max, diameter, r, g, b, a, duration);
	}
};