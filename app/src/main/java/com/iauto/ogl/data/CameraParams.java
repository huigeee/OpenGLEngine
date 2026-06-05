package com.iauto.ogl.data;

import android.os.Parcel;
import android.os.Parcelable;

public class CameraParams implements Parcelable {
    private Vec3 position;
    private Vec3 target;
    private float fov;
    private float nearPlane;
    private float farPlane;
    private boolean orthographic;
    private float orthoSize;

    public CameraParams() {
        this(new Vec3(4f, 0f, 5f), new Vec3(0f, 0.5f, 0f), 45f, 0.1f, 100f, false, 10f);
    }
    public CameraParams(Vec3 position, Vec3 target, float fov,
                        float nearPlane, float farPlane, boolean orthographic, float orthoSize) {
        this.position = position; this.target = target; this.fov = fov;
        this.nearPlane = nearPlane; this.farPlane = farPlane;
        this.orthographic = orthographic; this.orthoSize = orthoSize;
    }
    protected CameraParams(Parcel parcel) {
        ClassLoader cl = Vec3.class.getClassLoader();
        position = parcel.readParcelable(cl); if (position == null) position = new Vec3();
        target = parcel.readParcelable(cl); if (target == null) target = new Vec3();
        fov = parcel.readFloat(); nearPlane = parcel.readFloat();
        farPlane = parcel.readFloat(); orthographic = parcel.readByte() != 0;
        orthoSize = parcel.readFloat();
    }

    public Vec3 getPosition() { return position; }
    public void setPosition(Vec3 v) { this.position = v; }
    public Vec3 getTarget() { return target; }
    public void setTarget(Vec3 v) { this.target = v; }
    public float getFov() { return fov; }
    public void setFov(float v) { this.fov = v; }
    public float getNearPlane() { return nearPlane; }
    public void setNearPlane(float v) { this.nearPlane = v; }
    public float getFarPlane() { return farPlane; }
    public void setFarPlane(float v) { this.farPlane = v; }
    public boolean getOrthographic() { return orthographic; }
    public void setOrthographic(boolean v) { this.orthographic = v; }
    public float getOrthoSize() { return orthoSize; }
    public void setOrthoSize(float v) { this.orthoSize = v; }

    @Override public void writeToParcel(Parcel dest, int flags) {
        dest.writeParcelable(position, flags); dest.writeParcelable(target, flags);
        dest.writeFloat(fov); dest.writeFloat(nearPlane); dest.writeFloat(farPlane);
        dest.writeByte((byte) (orthographic ? 1 : 0)); dest.writeFloat(orthoSize);
    }
    @Override public int describeContents() { return 0; }

    public static final Parcelable.Creator<CameraParams> CREATOR = new Parcelable.Creator<CameraParams>() {
        @Override public CameraParams createFromParcel(Parcel source) { return new CameraParams(source); }
        @Override public CameraParams[] newArray(int size) { return new CameraParams[size]; }
    };
}
