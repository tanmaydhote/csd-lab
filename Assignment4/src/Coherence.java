import java.util.Random;
import java.io.File;

class Coherence {
	static int blockSize = 32;
	static int cacheSize = 1024;
	static int numberOfProcessors = 2;
	static int numberOfIterations = 1000000;

	public static void main(String[] args) {
		runMESI();
		runMOESI();
	}

	public static void runMESI() {
		Random rand = new Random(900);
		int selectedProcessor;
		int otherProcessor;
		int blockNumber;
		int selMemBlock;
		int coherenceAccesses = 0;
		Processor[] processorArray = new Processor[numberOfProcessors];
		for (int i = 0; i < numberOfProcessors; i++) {
			processorArray[i] = new Processor(cacheSize, blockSize);
		}
		for (int i = 0; i < numberOfIterations; i++) {
			// randomly choosing a processor
			selectedProcessor = rand.nextInt(numberOfProcessors);
			processorArray[selectedProcessor].accessCounter++;
			// assuming 2 counters
			otherProcessor = 1 - selectedProcessor;
			// generate a read/write interest at a random address
			processorArray[selectedProcessor].generateInstruction();
			selMemBlock = processorArray[selectedProcessor].selectedBlock;
			// find the block in cache that the memory block will be put in
			blockNumber = processorArray[selectedProcessor].selectedBlock % 32;

			// For Read Instruction
			if (processorArray[selectedProcessor].read == true) {
				// System.out.println("Processor " + selectedProcessor +
				// " reads memory location "
				// + selMemBlock);
				// Read Hit
				if (processorArray[selectedProcessor].cache.blockArray[blockNumber].state != 'I'
						&& processorArray[selectedProcessor].cache.blockArray[blockNumber].tag == selMemBlock) {
					// do nothing
				} else
				// Read Miss
				{
					coherenceAccesses++;
					// If any other processor has it
					if (processorArray[otherProcessor].cache.blockArray[blockNumber].state != 'I'
							&& processorArray[otherProcessor].cache.blockArray[blockNumber].tag == selMemBlock) {
						// it can be either M or E
						// In both cases, make shared for both
						processorArray[selectedProcessor].stateChanges++;
						processorArray[otherProcessor].stateChanges++;
						processorArray[selectedProcessor].cache.blockArray[blockNumber].state = 'S';
						processorArray[otherProcessor].cache.blockArray[blockNumber].state = 'S';
					} else
					// If no other processor has it, directly fetch from memory
					{
						if (processorArray[selectedProcessor].cache.blockArray[blockNumber].state != 'E') 
						{
							processorArray[selectedProcessor].stateChanges++;
						}
						processorArray[selectedProcessor].cache.blockArray[blockNumber].state = 'E';
					}
				}
			} // Write Instruction
			else if (processorArray[selectedProcessor].read == false) {
				// System.out.println("Processor " + selectedProcessor +
				// " writes memory location "
				// + selMemBlock);
				// Write Hit
				if (processorArray[selectedProcessor].cache.blockArray[blockNumber].state != 'I'
						&& processorArray[selectedProcessor].cache.blockArray[blockNumber].tag == selMemBlock) {
					// If M, still remains M
					if (processorArray[selectedProcessor].cache.blockArray[blockNumber].state == 'E') {
						processorArray[selectedProcessor].cache.blockArray[blockNumber].state = 'M';
						processorArray[selectedProcessor].stateChanges++;
					} else if (processorArray[selectedProcessor].cache.blockArray[blockNumber].state == 'S') {
						coherenceAccesses++;
						processorArray[selectedProcessor].stateChanges++;
						processorArray[otherProcessor].stateChanges++;
						processorArray[selectedProcessor].cache.blockArray[blockNumber].state = 'M';
						processorArray[otherProcessor].cache.blockArray[blockNumber].state = 'I';
					}
				} else // Write Miss
				{
					coherenceAccesses++;
					// if someone else has it
					if (processorArray[otherProcessor].cache.blockArray[blockNumber].state != 'I'
							&& processorArray[otherProcessor].cache.blockArray[blockNumber].tag == selMemBlock) {
						processorArray[selectedProcessor].stateChanges++;
						processorArray[otherProcessor].stateChanges++;
						processorArray[otherProcessor].cache.blockArray[blockNumber].state = 'I';
						processorArray[selectedProcessor].cache.blockArray[blockNumber].state = 'M';

					} else {				
						
						if (processorArray[selectedProcessor].cache.blockArray[blockNumber].state != 'M')
						{
							processorArray[selectedProcessor].stateChanges++;
						}
							processorArray[selectedProcessor].cache.blockArray[blockNumber].state = 'M';
					}
				}
				// System.out.println("Instruction is" +
				// processorArray[selectedProcessor].selectedBlock +
				// " "+processorArray[selectedProcessor].read+selectedProcessor);

			}
			// updating value of the cache block
			processorArray[selectedProcessor].cache.blockArray[blockNumber].tag = selMemBlock;
		}

		System.out.println("-----------MESI----------------");
		System.out.println("For processor 0");
		System.out.println("Number of accesses "
				+ processorArray[0].accessCounter);
		System.out.println("Number of state changes "
				+ processorArray[0].stateChanges);
		System.out.println("For processor 1");
		System.out.println("Number of accesses "
				+ processorArray[1].accessCounter);
		System.out.println("Number of state changes "
				+ processorArray[1].stateChanges);
		System.out.println("Number of coherence accesses " + coherenceAccesses);
	}

	// //////////////////////////////////////////////////////////////////////////////////////////////////
	// MESI ends here

	public static void runMOESI() {
		Random rand = new Random(100);
		int selectedProcessor;
		int otherProcessor;
		int blockNumber;
		int selMemBlock;
		int coherenceAccesses = 0;
		Processor[] processorArray = new Processor[numberOfProcessors];
		for (int i = 0; i < numberOfProcessors; i++) {
			processorArray[i] = new Processor(cacheSize, blockSize);
		}
		for (int i = 0; i < numberOfIterations; i++) {
			selectedProcessor = rand.nextInt(numberOfProcessors);
			processorArray[selectedProcessor].accessCounter++;
			otherProcessor = 1 - selectedProcessor;
			processorArray[selectedProcessor].generateInstruction();
			selMemBlock = processorArray[selectedProcessor].selectedBlock;
			blockNumber = processorArray[selectedProcessor].selectedBlock % 32;
			if (processorArray[selectedProcessor].read == true) {
				// System.out.println("Processor " + selectedProcessor +
				// " reads memory location "
				// + selMemBlock);
				if (processorArray[selectedProcessor].cache.blockArray[blockNumber].state != 'I'
						&& processorArray[selectedProcessor].cache.blockArray[blockNumber].tag == selMemBlock) {
					// do nothing
				} else {

					if (processorArray[otherProcessor].cache.blockArray[blockNumber].state != 'I'
							&& processorArray[otherProcessor].cache.blockArray[blockNumber].tag == selMemBlock) {
						// it can be either M or E
						if (processorArray[otherProcessor].cache.blockArray[blockNumber].state == 'M') {
							processorArray[selectedProcessor].stateChanges++;
							processorArray[otherProcessor].stateChanges++;
							processorArray[selectedProcessor].cache.blockArray[blockNumber].state = 'S';
							processorArray[otherProcessor].cache.blockArray[blockNumber].state = 'O';
						} else {
							coherenceAccesses++;
							processorArray[selectedProcessor].stateChanges++;
							processorArray[otherProcessor].stateChanges++;
							processorArray[selectedProcessor].cache.blockArray[blockNumber].state = 'S';
							processorArray[otherProcessor].cache.blockArray[blockNumber].state = 'O';

						}
					} else {
						coherenceAccesses++;
						if (processorArray[selectedProcessor].cache.blockArray[blockNumber].state != 'E') {
							processorArray[selectedProcessor].stateChanges++;
						}
						processorArray[selectedProcessor].cache.blockArray[blockNumber].state = 'E';
					}
				}
			} else if (processorArray[selectedProcessor].read == false) {
				// System.out.println("Processor " + selectedProcessor +
				// " writes memory location "
				// + selMemBlock);
				if (processorArray[selectedProcessor].cache.blockArray[blockNumber].state != 'I'
						&& processorArray[selectedProcessor].cache.blockArray[blockNumber].tag == selMemBlock) {
					// If M still remains M
					if (processorArray[selectedProcessor].cache.blockArray[blockNumber].state == 'E') {
						processorArray[selectedProcessor].cache.blockArray[blockNumber].state = 'M';
						processorArray[selectedProcessor].stateChanges++;
					} else if (processorArray[selectedProcessor].cache.blockArray[blockNumber].state == 'S'
							|| processorArray[selectedProcessor].cache.blockArray[blockNumber].state == 'O') {
						coherenceAccesses++;
						processorArray[selectedProcessor].stateChanges++;
						processorArray[otherProcessor].stateChanges++;
						processorArray[selectedProcessor].cache.blockArray[blockNumber].state = 'M';
						processorArray[otherProcessor].cache.blockArray[blockNumber].state = 'I';
					}
				} else {
					coherenceAccesses++;
					if (processorArray[otherProcessor].cache.blockArray[blockNumber].state != 'I'
							&& processorArray[otherProcessor].cache.blockArray[blockNumber].tag == selMemBlock) {
						processorArray[selectedProcessor].stateChanges++;
						processorArray[otherProcessor].stateChanges++;
						processorArray[otherProcessor].cache.blockArray[blockNumber].state = 'I';
						processorArray[selectedProcessor].cache.blockArray[blockNumber].state = 'M';

					} else {
						if (processorArray[selectedProcessor].cache.blockArray[blockNumber].state != 'M') {
							processorArray[selectedProcessor].stateChanges++;
						}
						processorArray[selectedProcessor].cache.blockArray[blockNumber].state = 'M';
					}
				}
			}
			processorArray[selectedProcessor].cache.blockArray[blockNumber].tag = selMemBlock;
			// printCacheContents(processorArray);
		}

		System.out.println("-----------MOESI----------------");
		System.out.println("For processor 0");
		System.out.println("Number of accesses "
				+ processorArray[0].accessCounter);
		System.out.println("Number of state changes "
				+ processorArray[0].stateChanges);
		System.out.println("For processor 1");
		System.out.println("Number of accesses "
				+ processorArray[1].accessCounter);
		System.out.println("Number of state changes "
				+ processorArray[1].stateChanges);
		System.out.println("Number of coherence accesses " + coherenceAccesses);
	}

	public static void printCacheContents(Processor[] processorArray) {
		for (int i = 0; i < processorArray.length; i++) {
			System.out.println("Processor " + i);
			for (int j = 0; j < processorArray[i].cache.blockArray.length; j++) {
				System.out.println("Block Number " + j + " "
						+ processorArray[i].cache.blockArray[j].state + " "
						+ processorArray[i].cache.blockArray[j].tag + " ");

			}
			System.out.println("Number of accesses "
					+ processorArray[i].accessCounter);
			System.out.println("Number of state changes "
					+ processorArray[i].stateChanges);
		}
	}

}