#include "stdafx.h"
#include "DxLib.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>


int ProcessLoop();

void CameraRotationKey(float &cameraHAngle, float &cameraVAngle);
void PlayerMoveKey(float &angle, float &cameraHAngle, VECTOR &moveVector, int &animIndexToBeAttached);
void CalPlayerPosition(int animIndexToBeAttached, float cameraHAngle, VECTOR* position, VECTOR moveVector);
void MV1DrawPositionedModel(int playerModelHandle, float angle, VECTOR position);
void SetCameraPosition(VECTOR position, float cameraVAngle, float cameraHAngle);
void AnimChanger(int animIndexToBeAttached, int *animIndexAttached, int *attachIndex, int playerModelHandle, float *playTime, float *totalTime);
void DrawLattice();


typedef enum {
	menu,config,  game, score
}playMode;

//1フリップあたりの移動量
#define MOVESPEED 10.0f;
// 待機アニメーションの番号
#define ANIM_NEUTRAL 0
//走りアニメーションの番号
#define ANIM_RUN 1
//ジャンプアニメーションの番号
#define ANIM_JUMP 2
//攻撃アニメーションの番号
#define ANIM_ATTACK 3
//１フリップ当たりのカメラの回転角度
#define CAMERA_ANGLE_SPEED 2.0f
//カメラの注視点の高さ
#define CAMERA_LOOK_AT_HEIGHT 15.0f
//カメラと注視点の距離
#define CAMERA_LOOK_AT_DISTANCE 100.0f
// フィールドラインを描く範囲
#define LINE_AREA_SIZE			2400.0f
// ラインの数
#define LINE_NUM			24

//敵の数
#define GHOST_NUMBER  15
//敵の動く速さ
#define GHOST_MOVESPEED 9.0f
//敵の出現時距離
#define GHOST_DISTANCE 400.0f
//敵の出現間隔時間
#define GHOST_TIME 30
//敵の透過度合い
#define GHOST_TRANSPARENT_RATE 0.45f






int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	static int nowMode; //現在の画面(シーン)

	//プレイヤーのモデルとアニメ関係
	int playerModelHandle, attachIndex, animIndexToBeAttached, animIndexAttached;
	VECTOR position, moveVector;
	float angle, totalTime, playTime;
	//背景関連
	int bgModelHandle1, bgModelHandle2, bgModelHandle3, bgModelHandle4, bgModelHandle5;
	//カメラ関係
	float  cameraHAngle, cameraVAngle;
	//敵関連
	int ghostCounter, ghostModelHandle[GHOST_NUMBER];
	float ghostTotalTime, ghostPositionX, ghostPositionZ;
	float ghostAngle[GHOST_NUMBER], ghostPlayTime[GHOST_NUMBER];
	VECTOR ghostPosition[GHOST_NUMBER], ghostMoveVector[GHOST_NUMBER], ghostToPlayer[GHOST_NUMBER];
	//他
	int debagModeFlag;
	char key[256]; //キーの押下状態を格納 
	char oldKey[256]; //前のフレームのキーの押下状態を格納

	//SetBackgroundColor(50, 155, 255)
	ChangeWindowMode(TRUE), SetWindowSizeChangeEnableFlag(TRUE), SetBackgroundColor(0, 0, 0), SetDrawScreen(DX_SCREEN_BACK);
	SetGraphMode(1000, 750, 32);
	if (DxLib_Init() == -1)	{return -1;}


	

	//バックグラウンドのモデルをロード
	bgModelHandle1 = MV1LoadModel("dat/アノマロさん/デッドマスターの部屋/デッドマスターの部屋.x");
	//bgModelHandle1 = MV1LoadModel("dat/jimenB6/Jimen_B601.x");
	bgModelHandle2 = MV1LoadModel("dat/packaged用ステージ1・始まりの場所/packaged用共通の空.x");
	MV1SetScale(bgModelHandle1, VGet(30.0f, 30.0f, 30.0f));
	MV1SetScale(bgModelHandle2, VGet(30.0f, 30.0f, 30.0f));
	

	//プレイヤーのモデルをロード、アタッチ済みのアニメの番号を存在しないものに、アニメの再生時間を初期化
	playerModelHandle = MV1LoadModel("dat/Lat式ミク/Lat式ミクVer2.3_Normalエッジ無し専用.pmd");
	animIndexAttached = 999;
	playTime = 0.0f;

	// カメラの向きを初期化
	cameraHAngle = 0.0f;
	cameraVAngle = -3.0f;
	SetCameraNearFar(15.0f, 3500.0f);

	// プレイヤーの向きと位置を初期化
	angle = 0.0f;
	position = VGet(0.0f, 0.0f, 0.0f);

	//敵のモデルをロード
	ghostModelHandle[0] = MV1LoadModel("dat/ミク三姉妹のお化け/ミクお化け/ミクお化けVer1.0.pmd");
	MV1SetOpacityRate(ghostModelHandle[0], GHOST_TRANSPARENT_RATE); //敵モデルの半透明化
	MV1SetScale(ghostModelHandle[0], VGet(5.0f, 5.0f, 5.0f));
	ghostPositionX = 0.0f;
	ghostPositionZ =1000.0f;
	ghostPosition[0] = VGet(ghostPositionX, 0.0f, ghostPositionZ);
	ghostAngle[0] = atan2(ghostPositionX, ghostPositionZ);
	

	//zバッファを有効に
	SetUseZBufferFlag(TRUE);
	SetWriteZBuffer3D(TRUE);
	MV1SetUseZBuffer(bgModelHandle2,TRUE);
	MV1SetWriteZBuffer(bgModelHandle2, TRUE);

	
	//その他初期化
	debagModeFlag = 0;
	ghostCounter = 1;
	
	int loopCounter = 0;
	//メインループ
	while (ProcessLoop() == 0) {
		loopCounter++;
		
		

		//前のフレームでの押下状態を保存、現フレームの押下状態を取得
		memcpy(oldKey, key, sizeof(key));
		GetHitKeyStateAll(key);

		//背景モデルを描画
		MV1DrawModel(bgModelHandle1);
		MV1DrawModel(bgModelHandle2);
		


		//プレイヤーとカメラの移動、描画
		{
			// Q,Rキー：カメラの水平角度を変更, W,Eキー：カメラの垂直角度を変更
			CameraRotationKey(cameraHAngle, cameraVAngle);

			// 移動ベクトルを初期化
			moveVector = VGet(0.0f, 0.0f, 0.0f);

			//方向キー：1フリップでのキャラクターの移動ベクトルと向きを設定, アニメーション番号を走りに設定
			PlayerMoveKey(angle, cameraHAngle, moveVector, animIndexToBeAttached);

			//プレイヤーの位置と向きを算出
			CalPlayerPosition(animIndexToBeAttached, cameraHAngle, &position, moveVector);

			//プレイヤーの位置と向きをセット後、描画
			MV1DrawPositionedModel(playerModelHandle, angle, position);

			// ３Ｄモデルの描画
			MV1DrawModel(playerModelHandle);

			// カメラの位置と向きを算出、セット
			SetCameraPosition(position, cameraVAngle, cameraHAngle);

			//必要ならアニメーションを変更、時間を進める
			AnimChanger(animIndexToBeAttached, &animIndexAttached, &attachIndex, playerModelHandle, &playTime, &totalTime);

		}
		

		//一定時間ごとに敵の数を増やす
		if (loopCounter%GHOST_TIME == 0 && ghostCounter <GHOST_NUMBER) {


			ghostModelHandle[ghostCounter] = MV1DuplicateModel(ghostModelHandle[0]);
			MV1SetOpacityRate(ghostModelHandle[ghostCounter], GHOST_TRANSPARENT_RATE); //敵モデルの半透明化
			MV1SetScale(ghostModelHandle[ghostCounter], VGet(4.0f, 4.0f, 4.0f));

			do {
				
				ghostPositionX = rand() % 2401 - 1200;
				ghostPositionZ = rand() % 2401 - 1200;
				ghostPosition[ghostCounter] = VGet(ghostPositionX, 0.0f, ghostPositionZ);
				ghostToPlayer[ghostCounter] = VSub(position, ghostPosition[ghostCounter]);
			} while (VSize(ghostToPlayer[ghostCounter]) <GHOST_DISTANCE  );

			ghostCounter++;
		}
		else if (loopCounter % GHOST_TIME == 0 && ghostCounter%GHOST_NUMBER >= 1 ) {
			
			MV1DeleteModel(ghostModelHandle[ghostCounter%GHOST_NUMBER]);
			ghostModelHandle[ghostCounter%GHOST_NUMBER] = MV1DuplicateModel(ghostModelHandle[0]);
			MV1SetOpacityRate(ghostModelHandle[ghostCounter%GHOST_NUMBER], GHOST_TRANSPARENT_RATE); //敵モデルの半透明化
			MV1SetScale(ghostModelHandle[ghostCounter%GHOST_NUMBER], VGet(4.0f, 4.0f, 4.0f));

			do {
				
				ghostPositionX = rand() % 2401 - 1200;
				ghostPositionZ = rand() % 2401 - 1200;
				ghostPosition[ghostCounter%GHOST_NUMBER] = VGet(ghostPositionX, 0.0f, ghostPositionZ);
				ghostToPlayer[ghostCounter%GHOST_NUMBER] = VSub(position, ghostPosition[ghostCounter%GHOST_NUMBER]);
			} while (VSize(ghostToPlayer[ghostCounter%GHOST_NUMBER]) < GHOST_DISTANCE);
			ghostCounter++;
		}
		else if (loopCounter % 40 == 0 && ghostCounter%GHOST_NUMBER ==0) {
			ghostCounter++;
		}

		if (ghostCounter < GHOST_NUMBER) {
			int d;
			for (d = 0; d < ghostCounter; d++) {
				// ３Ｄモデルに新しい位置座標をセット
				ghostToPlayer[d] = VSub(position, ghostPosition[d]);

				if (VSize(ghostToPlayer[d]) < 13) {
					SetFontSize(100);
					SetFontThickness(10);
					//ChangeFont("ＭＳ 明朝");                    
					//ChangeFontType(DX_FONTTYPE_ANTIALIASING);     //アンチエイリアスフォントに変更
					DrawFormatString(30, 550, GetColor(255, 0, 0), "GAME OVER");
					//DrawFormatString(500, 550, GetColor(255, 0, 0), "SCORE %d", loopCounter);
					
				}

				ghostMoveVector[d] = VGet(VNorm(ghostToPlayer[d]).x * GHOST_MOVESPEED, 0.0f, VNorm(ghostToPlayer[d]).z*GHOST_MOVESPEED);
				ghostPosition[d] = VAdd(ghostPosition[d], ghostMoveVector[d]);
				MV1SetPosition(ghostModelHandle[d], ghostPosition[d]);

				// ３Ｄモデルの描画	
				MV1DrawModel(ghostModelHandle[d]);

				//向きをプレイヤーのほうに
				ghostAngle[d] = atan2(ghostPosition[d].x - position.x, ghostPosition[d].z - position.z);
				MV1SetRotationXYZ(ghostModelHandle[d], VGet(0.0f, ghostAngle[d], 0.0f));

				//描画
				MV1DrawModel(ghostModelHandle[d]);
			}
		}
		else if (ghostCounter >= GHOST_NUMBER) {

			int d;
			for (d = 0; d < GHOST_NUMBER; d++) {
				// ３Ｄモデルに新しい位置座標をセット
				ghostToPlayer[d] = VSub(position, ghostPosition[d]);

				if (VSize(ghostToPlayer[d]) < 13) {
					SetFontSize(60);
					SetFontThickness(6);
					//ChangeFont("ＭＳ 明朝");                    
					//ChangeFontType(DX_FONTTYPE_ANTIALIASING);     //アンチエイリアスフォントに変更
					DrawString(30, 550, "GAME OVER", GetColor(255, 0, 0));
				}

				ghostMoveVector[d] = VGet(VNorm(ghostToPlayer[d]).x * GHOST_MOVESPEED, 0.0f, VNorm(ghostToPlayer[d]).z*GHOST_MOVESPEED);
				ghostPosition[d] = VAdd(ghostPosition[d], ghostMoveVector[d]);
				MV1SetPosition(ghostModelHandle[d], ghostPosition[d]);

				// ３Ｄモデルの描画	
				MV1DrawModel(ghostModelHandle[d]);

				//向きをプレイヤーのほうに
				ghostAngle[d] = atan2(ghostPosition[d].x - position.x, ghostPosition[d].z - position.z);
				MV1SetRotationXYZ(ghostModelHandle[d], VGet(0.0f, ghostAngle[d], 0.0f));

				//描画
				MV1DrawModel(ghostModelHandle[d]);
			}
		}

		

		//Lキーでデバッグモード(格子とか位置示すもの)
		if (key[KEY_INPUT_D] == 1 && oldKey[KEY_INPUT_D]==0) {
			if (debagModeFlag == 0) debagModeFlag = 1;
			else debagModeFlag = 0;
		}
		if (debagModeFlag == 1) DrawLattice();


		//SetFontSize(130);
		//SetFontThickness(15);
		//DrawFormatString(100, 0, GetColor(255, 0, 0), "SCORE %d", loopCounter);

	}

	
	DxLib_End();				

	return 0;				
}




//メインループの条件式で呼ぶ関数
int ProcessLoop() {
	if (ScreenFlip() != 0)return -1;
	if (ProcessMessage() != 0)return -1;
	if (CheckHitKey(KEY_INPUT_ESCAPE) != 0)return -1;
	if (clsDx() != 0)return -1;
	if (ClearDrawScreen() != 0)return -1;
	return 0;
}


// Q,Rキー：カメラの水平角度を変更, W,Eキー：カメラの垂直角度を変更

void CameraRotationKey(float &cameraHAngle, float &cameraVAngle) {
	if (CheckHitKey(KEY_INPUT_R) == 1) {
		cameraHAngle -= CAMERA_ANGLE_SPEED;
		if (cameraHAngle <= -180) {
			cameraHAngle += 360;
		}
	}
	if (CheckHitKey(KEY_INPUT_Q) == 1) {
		cameraHAngle += CAMERA_ANGLE_SPEED;
		if (cameraHAngle >= 180) {
			cameraHAngle -= 360;
		}
	}
	if (CheckHitKey(KEY_INPUT_E) == 1) {
		cameraVAngle -= CAMERA_ANGLE_SPEED;
		if (cameraVAngle <= -5.0f) {
			cameraVAngle = -5.0f;
		}
	}
	if (CheckHitKey(KEY_INPUT_W) == 1) {
		cameraVAngle += CAMERA_ANGLE_SPEED;
		if (cameraVAngle >= 89.0f) {
			cameraVAngle = 89.0f;
		}
	}
}



//方向キー：1フリップでのキャラクターの移動ベクトルと向きを設定, アニメーションナンバーを走りに設定
void PlayerMoveKey(float &angle, float &cameraHAngle, VECTOR &moveVector, int &animIndexToBeAttached) {
	if (CheckHitKey(KEY_INPUT_LEFT) == 1 && CheckHitKey(KEY_INPUT_DOWN) != 1 && CheckHitKey(KEY_INPUT_UP) != 1) {
		angle = 90.0f - cameraHAngle;
		moveVector.x = -MOVESPEED;
		animIndexToBeAttached = ANIM_RUN;	
	}
	else if (CheckHitKey(KEY_INPUT_LEFT) == 1 && CheckHitKey(KEY_INPUT_DOWN) == 1) {
		angle = 45.0f - cameraHAngle;
		moveVector.x = -cos(45.0 / 180.0f * DX_PI_F) * MOVESPEED;
		moveVector.z = -cos(45.0 / 180.0f * DX_PI_F) * MOVESPEED;
		animIndexToBeAttached = ANIM_RUN;
	}
	else if (CheckHitKey(KEY_INPUT_LEFT) == 1 && CheckHitKey(KEY_INPUT_UP) == 1) {
		angle = 135.0f - cameraHAngle;
		moveVector.x = -cos(45.0 / 180.0f * DX_PI_F) * MOVESPEED;
		moveVector.z = cos(45.0 / 180.0f * DX_PI_F) * MOVESPEED;
		animIndexToBeAttached = ANIM_RUN;
	}
	else if (CheckHitKey(KEY_INPUT_RIGHT) == 1 && CheckHitKey(KEY_INPUT_UP) != 1 && CheckHitKey(KEY_INPUT_DOWN) != 1) {
		angle = -90.0f - cameraHAngle;
		moveVector.x = MOVESPEED;
		animIndexToBeAttached = ANIM_RUN;
	}
	else if (CheckHitKey(KEY_INPUT_RIGHT) == 1 && CheckHitKey(KEY_INPUT_DOWN) == 1) {
		angle = -45.0f - cameraHAngle;
		moveVector.x = cos(45.0 / 180.0f * DX_PI_F) * MOVESPEED;
		moveVector.z = -cos(45.0 / 180.0f * DX_PI_F) * MOVESPEED;
		animIndexToBeAttached = ANIM_RUN;
	}
	else if (CheckHitKey(KEY_INPUT_RIGHT) == 1 && CheckHitKey(KEY_INPUT_UP) == 1) {
		angle = -135.0f - cameraHAngle;
		moveVector.x = cos(45.0 / 180.0f * DX_PI_F) * MOVESPEED;
		moveVector.z = cos(45.0 / 180.0f * DX_PI_F) * MOVESPEED;
		animIndexToBeAttached = ANIM_RUN;

	}
	else if (CheckHitKey(KEY_INPUT_UP) == 1 && CheckHitKey(KEY_INPUT_RIGHT) != 1 && CheckHitKey(KEY_INPUT_LEFT) != 1) {
		angle = 180.0f - cameraHAngle;
		moveVector.z = MOVESPEED;
		animIndexToBeAttached = ANIM_RUN;
	}
	else if (CheckHitKey(KEY_INPUT_DOWN) == 1 && CheckHitKey(KEY_INPUT_RIGHT) != 1 && CheckHitKey(KEY_INPUT_LEFT) != 1) {
		angle = 0.0f - cameraHAngle;
		moveVector.z = -MOVESPEED;
		animIndexToBeAttached = ANIM_RUN;
	}
	else if (CheckHitKey(KEY_INPUT_DOWN) != 1 && CheckHitKey(KEY_INPUT_RIGHT) != 1 && CheckHitKey(KEY_INPUT_LEFT) != 1 && CheckHitKey(KEY_INPUT_DOWN) != 1) {
		
		animIndexToBeAttached = ANIM_NEUTRAL;
	}

}


//position, movePosition, cameraHAngleから走るプレイヤーの位置と向きを算出
void CalPlayerPosition(int animIndexToBeAttached, float cameraHAngle, VECTOR* position, VECTOR moveVector) {

	//３dモデルの位置と向きを算出、セット
	if (animIndexToBeAttached == ANIM_RUN) {
		VECTOR tempMoveVector;
		// カメラの角度に合わせて移動ベクトルを回転してから加算
		float sinParam = sin(cameraHAngle / 180.0f * DX_PI_F);
		float cosParam = cos(cameraHAngle / 180.0f * DX_PI_F);
		tempMoveVector.x = moveVector.x * cosParam - moveVector.z * sinParam;
		tempMoveVector.y = 0.0f;
		tempMoveVector.z = moveVector.x * sinParam + moveVector.z * cosParam;
		*position = VAdd(*position, tempMoveVector);
	}
}

//3dモデルに位置と角度をセットして描画
void MV1DrawPositionedModel(int playerModelHandle, float angle,VECTOR position) {
	// 新しい向きをセット
	MV1SetRotationXYZ(playerModelHandle, VGet(0.0f, angle / 180.0f * DX_PI_F, 0.0f));
	// ３Ｄモデルに新しい位置座標をセット
	MV1SetPosition(playerModelHandle, position);
	// ３Ｄモデルの描画	
	MV1DrawModel(playerModelHandle);
}


// カメラの位置と向きを算出、セット
void SetCameraPosition(VECTOR position, float cameraVAngle, float cameraHAngle) {

	VECTOR TempPosition1;
	VECTOR TempPosition2;
	VECTOR CameraPosition;
	VECTOR CameraLookAtPosition;
	float sinParam, cosParam;


	// 注視点はキャラクターモデルの座標から CAMERA_LOOK_AT_HEIGHT 分だけ高い位置
	CameraLookAtPosition = position;
	CameraLookAtPosition.y += CAMERA_LOOK_AT_HEIGHT;

	// カメラの位置をカメラの水平角度と垂直角度から算出
	// 最初に垂直角度を反映した位置を算出
	sinParam = sin(cameraVAngle / 180.0f * DX_PI_F);
	cosParam = cos(cameraVAngle / 180.0f * DX_PI_F);
	TempPosition1.x = 0.0f;
	TempPosition1.y = sinParam * CAMERA_LOOK_AT_DISTANCE;
	TempPosition1.z = -cosParam * CAMERA_LOOK_AT_DISTANCE;

	// 次に水平角度を反映した位置を算出
	sinParam = sin(cameraHAngle / 180.0f * DX_PI_F);
	cosParam = cos(cameraHAngle / 180.0f * DX_PI_F);
	TempPosition2.x = cosParam * TempPosition1.x - sinParam * TempPosition1.z;
	TempPosition2.y = TempPosition1.y;
	TempPosition2.z = sinParam * TempPosition1.x + cosParam * TempPosition1.z;

	// 算出した座標に注視点の位置を加算したものがカメラの位置
	CameraPosition = VAdd(TempPosition2, CameraLookAtPosition);

	// カメラの設定に反映する
	SetCameraPositionAndTarget_UpVecY(CameraPosition, CameraLookAtPosition);
}


//アニメーションの時間をすすめる,もし必要ならアニメーションの変更
void AnimChanger(int animIndexToBeAttached, int *animIndexAttached, int *attachIndex, int playerModelHandle, float *playTime, float *totalTime) {
	// 今までと状態が変化した場合はアニメーションを変更
	if (animIndexToBeAttached != *animIndexAttached) {

		// 今までアタッチしていたアニメーションをデタッチ, 新しいアニメーションをアタッチ
		MV1DetachAnim(playerModelHandle, *attachIndex);
		if (animIndexToBeAttached == ANIM_RUN) {
			*attachIndex = MV1AttachAnim(playerModelHandle, ANIM_RUN, -1, FALSE);
			*animIndexAttached = ANIM_RUN;
		}
		else if (animIndexToBeAttached == ANIM_NEUTRAL) {
			*attachIndex = MV1AttachAnim(playerModelHandle, ANIM_NEUTRAL, -1, FALSE);
			*animIndexAttached = ANIM_NEUTRAL;
		}

		*totalTime = MV1GetAttachAnimTotalTime(playerModelHandle, *attachIndex);
		//playTime = 0.0f;
	}

	// 再生時間を進める
	*playTime += 1.5f;
	// 再生時間がアニメーションの総再生時間に達したらループ
	if (*playTime >= *totalTime) {
		*playTime = *playTime - *totalTime;
	}
	// 再生時間をセットする
	MV1SetAttachAnimTime(playerModelHandle, *attachIndex, *playTime);

}




// 地面に格子と座標軸を描画、球を描画(デバッグ用)
void DrawLattice() {

	int i, j;
	VECTOR Pos1;
	VECTOR Pos2;
	VECTOR Pos3;


	DrawLine3D(VGet(0, 1000, 0), VGet(0, -1000, 0), GetColor(255, 0, 0));
	DrawLine3D(VGet(1000, 0, 0.3f), VGet(-1000, 0, 0.3f), GetColor(255, 0, 0));
	DrawLine3D(VGet(0.3f, 0, 1000), VGet(0.3f, 0, -1000), GetColor(255, 0, 0));


	Pos1 = VGet(100, 0, 0);
	Pos2 = VGet(80, 5, 0);
	Pos3 = VGet(80, -5, 0);
	DrawTriangle3D(Pos1, Pos2, Pos3, GetColor(0, 255, 0), TRUE);


	Pos1 = VGet(0, 0, 100);
	Pos2 = VGet(0, 5, 80);
	Pos3 = VGet(0, -5, 80);
	DrawTriangle3D(Pos1, Pos2, Pos3, GetColor(0, 200, 255), TRUE);




	Pos1 = VGet(-LINE_AREA_SIZE / 2.0f, 0.0f, -LINE_AREA_SIZE / 2.0f);
	Pos2 = VGet(-LINE_AREA_SIZE / 2.0f, 0.0f, LINE_AREA_SIZE / 2.0f);
	for (i = 0; i <= LINE_NUM; i++) {

		if (i == 15) {
			Pos1.x += LINE_AREA_SIZE / LINE_NUM;
			Pos2.x += LINE_AREA_SIZE / LINE_NUM;
			continue;
		}


		DrawLine3D(Pos1, Pos2, GetColor(0, 200, 255));
		Pos1.x += LINE_AREA_SIZE / LINE_NUM;
		Pos2.x += LINE_AREA_SIZE / LINE_NUM;
	}

	Pos1 = VGet(-LINE_AREA_SIZE / 2.0f, 0.0f, -LINE_AREA_SIZE / 2.0f);
	Pos2 = VGet(LINE_AREA_SIZE / 2.0f, 0.0f, -LINE_AREA_SIZE / 2.0f);
	for (i = 0; i < LINE_NUM; i++) {

		if (i == 15) {
			Pos1.z += LINE_AREA_SIZE / LINE_NUM;
			Pos2.z += LINE_AREA_SIZE / LINE_NUM;
			continue;
		}

		DrawLine3D(Pos1, Pos2, GetColor(0, 255, 200));
		Pos1.z += LINE_AREA_SIZE / LINE_NUM;
		Pos2.z += LINE_AREA_SIZE / LINE_NUM;
	}


	//球の描画
	DrawSphere3D(VGet(LINE_AREA_SIZE / 2.0f, 3.0f, LINE_AREA_SIZE / 2.0f), 20.0f, 128, GetColor(255, 255, 255), GetColor(255, 255, 255), TRUE);
	DrawSphere3D(VGet(LINE_AREA_SIZE / 2.0f, 3.0f, -LINE_AREA_SIZE / 2.0f), 20.0f, 128, GetColor(255, 255, 255), GetColor(255, 255, 255), TRUE);
	DrawSphere3D(VGet(-LINE_AREA_SIZE / 2.0f, 3.0f, -LINE_AREA_SIZE / 2.0f), 20.0f, 128, GetColor(255, 255, 255), GetColor(255, 255, 255), TRUE);
	DrawSphere3D(VGet(-LINE_AREA_SIZE / 2.0f, 3.0f, LINE_AREA_SIZE / 2.0f), 20.0f, 128, GetColor(255, 255, 255), GetColor(255, 255, 255), TRUE);


}