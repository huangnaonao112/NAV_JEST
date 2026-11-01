/*
 Navicat MySQL Data Transfer

 Source Server         : localhost
 Source Server Type    : MySQL
 Source Server Version : 50731 (5.7.31-log)
 Source Host           : localhost:3306
 Source Schema         : 11

 Target Server Type    : MySQL
 Target Server Version : 50731 (5.7.31-log)
 File Encoding         : 65001

 Date: 28/04/2026 16:37:16
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for tb_userinfo
-- ----------------------------
DROP TABLE IF EXISTS `tb_userinfo`;
CREATE TABLE `tb_userinfo`  (
  `USER_ID` varchar(20) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
  `USER_NAME` varchar(50) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `USER_PASSWORD` varchar(20) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL,
  `USER_LEVEL` int(11) NULL DEFAULT NULL,
  `phone` varchar(32) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT '' COMMENT '电话',
  `dept` varchar(64) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT '' COMMENT '部门',
  `status` varchar(32) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT '正常' COMMENT '状态',
  `updated_at` datetime NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`USER_ID`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8 COLLATE = utf8_bin ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of tb_userinfo
-- ----------------------------

SET FOREIGN_KEY_CHECKS = 1;
