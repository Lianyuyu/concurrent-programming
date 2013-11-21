import java.util.concurrent.Semaphore;

public class DiningPhilosophers {

	/**
	 * @Lian Yu, Dec 12, 2012
	 */
	// static int phiNum = 5;
	// static int forkNum = 5;
	// static int targetTurn = 100;
	// static long[][] waitTimes = new long[phiNum][targetTurn];
	// static int[] runnedTurn = new int[phiNum];
	// static Thread[] threads = new Thread[phiNum];

	public static void main(String[] args) {
		
		if(args.length != 4){
			System.out.println("The number of arguments is wrong.");
			return;
		}
		for(int i =0; i < args.length ;i++){
			if(Integer.parseInt(args[i]) == 0){
				System.out.println("Argument can not be equal to 0.");
				return;
			}
		}
		
		int phiNum = Integer.parseInt(args[0]);
		int thinkMinTime = Integer.parseInt(args[1]);
		int thinkMaxTime = Integer.parseInt(args[2]);
		int eatTime = Integer.parseInt(args[3]);
		int forkNum = phiNum;
		
//		int thinkMinTime = 1;
//		int thinkMaxTime = 1;
//		int eatTime = 1;
//		int phiNum = 6;
//		int forkNum = phiNum;
		
		if(thinkMinTime > thinkMaxTime){
			System.out.println("thinkMaxTime must be bigger than thinkMinTime");
			return;
		}
				
		int targetTurn = 1000;
		long[][] waitTimes = new long[phiNum][targetTurn];
		int[] runnedTurn = new int[phiNum];
		Thread[] threads = new Thread[phiNum];
		
		//init fork and start threads
		Fork fork = new Fork(forkNum);
		for (int i = 0; i < phiNum; i++) {
			threads[i] = new Thread(new Philosopher(i, fork, thinkMinTime,
					thinkMaxTime, eatTime, waitTimes, phiNum, runnedTurn,
					targetTurn, threads));
			threads[i].start();
		}

		// Thread t = new Thread(new CheckThread(fork, phiNum));
		// t.start();
		
		//wait for all threads exit
		for (int i = 0; i < phiNum; i++) {
			try {
				threads[i].join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}

		System.out.println("code end");
	}
}

class Fork {

	private Semaphore[] sem;
	private int num;
	private int[] state;

	public Fork(int num) {
		this.num = num;
		this.initSemaphore();
		this.state = new int[num];
		this.initState();
	}

	private void initSemaphore() {
		sem = new Semaphore[num];
		for (int i = 0; i < num; i++) {
			sem[i] = new Semaphore(1);
		}
	}

	public boolean pickupLeftFork(int phiId) {
		// check the test of both fork

		int forkId;
		forkId = phiId;
		boolean result = sem[forkId].tryAcquire();
		if (result) {
			return true;
		} else {
			return false;
		}
		// this.outputAcquireInfo(phiId, forkId);
	}

	public boolean pickupRightFork(int phiId) {
		int forkId;
		boolean result;
		if (phiId == (num - 1)) {
			forkId = 0;
		} else {
			forkId = phiId + 1;			
		}
		result = sem[forkId].tryAcquire();
		
		if (result) {
			return true;
		} else {
			return false;
		}
		// this.outputAcquireInfo(phiId, forkId);
	}

	public void putLeftFork(int phiId) {
		int forkId = phiId;
		sem[forkId].release();
		// this.outputReleaseInfo(phiId, forkId);
	}

	public void putRightFork(int phiId) {
		int forkId;
		if (phiId == (num - 1)) {
			forkId = 0;
		} else {
			forkId = phiId + 1;			
		}
		sem[forkId].release();
		// this.outputReleaseInfo(phiId, forkId);
	}

	//not be used
	public void checkState(int phiId) {
		int leftFork, rightFork;
		if (phiId == (num - 1)) {
			leftFork = phiId;
			rightFork = 0;
		} else {
			leftFork = phiId;
			rightFork = phiId + 1;
		}
		if (state[leftFork] == 1 && state[rightFork] == 1) {
			int random = (int) Math.random();
			try {
				Thread.sleep(random);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}

	public void outputAcquireInfo(int phiId, int forkId) {
		System.out.println("Philosopher " + phiId + " acquire sem " + forkId
				+ ".");
	}

	public void outputReleaseInfo(int phiId, int forkId) {
		System.out.println("Philosopher " + phiId + " release sem " + forkId
				+ ".");
	}

	public void outputSemInfo(int forkId) {
		if (this.sem[forkId].availablePermits() == 0) {
			System.out.println("Semaphore " + forkId + " is acquired.");
		} else {
			System.out.println("Semaphore " + forkId + " is available.");
		}
	}

	private void initState() {
		for (int i = 0; i < this.num; i++) {
			state[i] = 0;
		}
	}
	
	//release all holding forks of given philosopher ID
	public void releaseAllHoldingFork(int phiId){
		int rightForkId;
		if (phiId == (num - 1)) {
			rightForkId = 0;
		} else {
			rightForkId = phiId + 1;
		}
		if(sem[rightForkId].availablePermits() == 0){
			sem[rightForkId].release();
		}
		
		int leftforkId = phiId;
		sem[leftforkId].release();
		if(sem[leftforkId].availablePermits() == 0){
			sem[leftforkId].release();
		}
	}
}

class Philosopher extends Thread {

	private int Id;
	private Fork fork;
	private int thinkMinTime;
	private int thinkMaxTime;
	private int eatTime;
	private long[][] waitTimes;
	private int phiAllNum;
	private int[] runnedTurn;
	private int targetTurn;
	private Thread[] threads;

	@SuppressWarnings("finally")
	@Override
	public void run() {
		try {
			System.out.println("thread " + Id + " starts.");
			boolean rightFork;
			boolean leftFork;
			int waitTimeRound = 0;
			long waitTimeBegin = 0;
			long waitTiemEnd;
			long totalTime;
			boolean refreshWaitTime = true;
			runnedTurn[Id] = 0;
			while (true) {
				if (isInterrupted()) {
					System.out.println("~~~~~~~~~~phi "+Id+" is interrupted.");
					return;
				}

				if (refreshWaitTime) {
					refreshWaitTime = false;
					waitTimeBegin = System.currentTimeMillis();
				}
				// try to eat
				leftFork = fork.pickupLeftFork(Id);
				if (leftFork) {
					rightFork = fork.pickupRightFork(Id);
					if (rightFork) {
						
						//this part of code is for counting efficiency
//						waitTiemEnd = System.currentTimeMillis();
//						totalTime = waitTiemEnd - waitTimeBegin;
//						waitTimes[Id][waitTimeRound] = totalTime;
//						waitTimeRound = waitTimeRound + 1;
//						runnedTurn[Id] = runnedTurn[Id] + 1;
//						refreshWaitTime = true;
//
//						if (waitTimeRound == targetTurn) {
//							for (int i = 0; i < phiAllNum; i++) {
//								if (i != Id) {
//									threads[i].interrupt();
//									System.out.println("phi "+i+" is called with interrupted.");
//								}
//							}
//							countEffciency(Id, waitTimes, runnedTurn,
//									targetTurn);
//							//clean up
//							fork.releaseAllHoldingFork(Id);
//							return;
//						}

						// start to eat
						this.Eat(eatTime);
						fork.releaseAllHoldingFork(Id);
						this.Think(thinkMinTime, thinkMaxTime);
					} else {
						fork.putLeftFork(Id);
					}
				}
			}
			
//		} catch (InterruptedException e) {
//			System.out.println("phi "+Id +" sleep interrupted.");
//			e.printStackTrace();
		} finally {
			//clean up
			fork.releaseAllHoldingFork(Id);
			return;
		}
	}

	public Philosopher(int Id, Fork fork, int thinkMinTime, int thinkMaxTime,
			int eatTime, long[][] waitTimes, int phiAllNum, int[] runnedTurn,
			int targetTurn, Thread[] threads) {
		this.Id = Id;
		this.fork = fork;
		this.eatTime = eatTime;
		this.thinkMaxTime = thinkMaxTime;
		this.thinkMinTime = thinkMinTime;
		this.waitTimes = waitTimes;
		this.phiAllNum = phiAllNum;
		this.runnedTurn = runnedTurn;
		this.targetTurn = targetTurn;
		this.threads = threads;
	}

	public int Think(int thinkMinTime, int thinkMaxTime)
			throws InterruptedException {
		int random = thinkMinTime
				+ (int) (Math.random() * (thinkMaxTime - thinkMinTime + 1));

		Thread.sleep(random);
		System.out.println("Philosopher " + this.Id + " is thinking.");

		return random;
	}

	public void Eat(int eatTime) throws InterruptedException {
		Thread.sleep(eatTime);
		System.out.println("Philosopher " + this.Id + " is eating.");

	}

	//count efficiency, need target turn to finish the loop, 
	public void countEffciency(int phiId, long[][] waitTimes, int[] runnedTurn,
			int targetTurn) {
		long sum;
		double sumEfficency = 0;
		double efficency[][] = new double[phiAllNum][targetTurn];
		System.out.println("philosopher " + phiId + " reach target turn");
		for (int i = 0; i < phiAllNum; i++) {
			sum = 0;
			sumEfficency = 0;
			for (int j = 0; j < runnedTurn[i]; j++) {
				sum = sum + waitTimes[i][j];
				efficency[i][j] = eatTime / (waitTimes[i][j] + eatTime);
				sumEfficency = sumEfficency + efficency[i][j];
			}
			double averageEfficency = sumEfficency / runnedTurn[i];
			System.out.println("phi " + i + " ran " + runnedTurn[i]
					+ " turns, the sum wait time is " + sum + ", efficency is "
					+ averageEfficency);
		}
	}
}


//for checking the semaphore condition, not be used
class CheckThread implements Runnable {

	private Fork fork;
	private int num;

	@Override
	public void run() {
		while (true) {
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
			for (int i = 0; i < num; i++) {
				fork.outputSemInfo(i);
			}
		}
	}

	public CheckThread(Fork fork, int num) {
		this.fork = fork;
		this.num = num;
	}
}
