package com.hjq.permissions;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Environment;
import android.provider.Settings;

/**
 *    author : Android 轮子哥
 *    github : https://github.com/getActivity/XXPermissions
 *    time   : 2022/06/11
 *    desc   : Android 11 权限委托实现
 */
class PermissionDelegateImplV30 extends PermissionDelegateImplV29 {

   @Override
   public boolean isGrantedPermission(Context context, String permission) {
      if (PermissionUtils.equalsPermission(permission, Permission.MANAGE_EXTERNAL_STORAGE)) {
         return isGrantedManageStoragePermission();
      }
      return super.isGrantedPermission(context, permission);
   }

   @Override
   public boolean isPermissionPermanentDenied(Activity activity, String permission) {
      if (PermissionUtils.equalsPermission(permission, Permission.MANAGE_EXTERNAL_STORAGE)) {
         return false;
      }
      return super.isPermissionPermanentDenied(activity, permission);
   }

   @Override
   public Intent getPermissionIntent(Context context, String permission) {
      if (PermissionUtils.equalsPermission(permission, Permission.MANAGE_EXTERNAL_STORAGE)) {
         return getManageStoragePermissionIntent(context);
      }
      return super.getPermissionIntent(context, permission);
   }

   /**
    * 是否有所有文件的管理权限
    */
   private static boolean isGrantedManageStoragePermission() {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
         return Environment.isExternalStorageManager();
      }
      return false;
   }

   /**
    * 获取所有文件的管理权限设置界面意图
    */
   private static Intent getManageStoragePermissionIntent(Context context) {
      Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
      intent.setData(PermissionUtils.getPackageNameUri(context));

      if (!PermissionUtils.areActivityIntent(context, intent)) {
         intent = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
      }

      if (!PermissionUtils.areActivityIntent(context, intent)) {
         intent = PermissionUtils.getApplicationDetailsIntent(context);
      }
      return intent;
   }
}