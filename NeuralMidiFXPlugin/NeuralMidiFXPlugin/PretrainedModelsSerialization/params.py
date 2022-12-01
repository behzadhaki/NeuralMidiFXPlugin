model_params = {
    'model_1':
        {
            # This is the Light version in MonotonicGrooveTransformer camomile vst
            # https://wandb.ai/mmil_tap2drum/transformer_groove_tap2drum/runs/vxuuth1y
            'pyModelPath': "pyTorch_models/misunderstood_bush_246-epoch_26.Model",
            'd_model': 128,
            'dim_ff': 128,
            'dropout': 0.1038,
            'n_heads': 4,
            'n_layers': 11,
            'embedding_sz': 27,
            'max_len': 32,
            'device': 'cpu'
        },
    'model_2':
        {
            # This is the Heavy version in MonotonicGrooveTransformer camomile vst
            # https://wandb.ai/mmil_tap2drum/transformer_groove_tap2drum/runs/2cgu9h6u
            'pyModelPath': "pyTorch_models/rosy_durian_248-epoch_26.Model",
            'd_model': 512,
            'dim_ff': 16,
            'dropout': 0.1093,
            'n_heads': 4,
            'n_layers': 6,
            'embedding_sz': 27,
            'max_len': 32,
            'device': 'cpu'
        },
    'model_3':
        {   # https://wandb.ai/mmil_tap2drum/transformer_groove_tap2drum/runs/v7p0se6e
            'pyModelPath': "pyTorch_models/hopeful_gorge_252-epoch_90.Model",
            'd_model': 512,
            'dim_ff': 64,
            'dropout': 0.1093,
            'n_heads': 4,
            'n_layers': 8,
            'embedding_sz': 27,
            'max_len': 32,
            'device': 'cpu'
        },
    'model_4':
        {
            # https://wandb.ai/mmil_tap2drum/transformer_groove_tap2drum/runs/35c9fysk
            'pyModelPath': "pyTorch_models/solar_shadow_247-epoch_41.Model",
            'd_model': 128,
            'dim_ff': 16,
            'dropout': 0.159,
            'n_heads': 1,
            'n_layers': 7,
            'embedding_sz': 27,
            'max_len': 32,
            'device': 'cpu'
        }
}
