#pragma once
#include "Marionnette.h"

class { SCRIPT_NAME } : public Marionnette
{
public:
	// コンストラクタ
    { SCRIPT_NAME }(GameObject& object) : Marionnette(object) {}
    // 初期化処理
    void Start() override;
	// 毎フレーム処理
    void Update() override;
};