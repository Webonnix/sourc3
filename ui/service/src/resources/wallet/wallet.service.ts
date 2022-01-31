/* eslint-disable @typescript-eslint/no-unused-vars */
import {
  exportOwnerKey,
  killApiServer,
  removeWallet, restoreExistedWallet, runWalletApi
} from './wallet.repository';

type ProcessStatus = {
  isOk: boolean,
  message: string
};

export const removeExistedWallet = async () => {
  const isWalletRemoved = await removeWallet();
  return isWalletRemoved;
};

export const restoreWallet = async (
  seed:string,
  password: string
):Promise<ProcessStatus> => {
  try {
    await removeWallet();
    const isRestored = await restoreExistedWallet(seed, password);
    return { isOk: true, message: isRestored };
  } catch (error) {
    const { message } = error as Error;
    return { isOk: false, message };
  }
};

export const enterUser = async (password:string):Promise<ProcessStatus> => {
  try {
    const ownerKey = await exportOwnerKey(password);
    console.log('owner key: ', ownerKey);
    const runWallet = await runWalletApi(password);
    return { isOk: true, message: runWallet };
  } catch (error) {
    const { message } = error as Error;
    return { isOk: false, message };
  }
};

export const killApi = async () => killApiServer();
