package com.iauto.ogl.data;

import android.os.Parcel;
import android.os.Parcelable;

public class Vec3 implements Parcelable {
    private float x;
    private float y;
    private float z;

    public Vec3() { this(0f, 0f, 0f); }
    public Vec3(float x, float y, float z) { this.x = x; this.y = y; this.z = z; }
    protected Vec3(Parcel parcel) {
        x = parcel.readFloat();
        y = parcel.readFloat();
        z = parcel.readFloat();
    }

    public float getX() { return x; }
    public float getY() { return y; }
    public float getZ() { return z; }
    public void setX(float x) { this.x = x; }
    public void setY(float y) { this.y = y; }
    public void setZ(float z) { this.z = z; }

    @Override public void writeToParcel(Parcel dest, int flags) {
        dest.writeFloat(x); dest.writeFloat(y); dest.writeFloat(z);
    }
    @Override public int describeContents() { return 0; }

    public static final Parcelable.Creator<Vec3> CREATOR = new Parcelable.Creator<Vec3>() {
        @Override public Vec3 createFromParcel(Parcel source) { return new Vec3(source); }
        @Override public Vec3[] newArray(int size) { return new Vec3[size]; }
    };
}
