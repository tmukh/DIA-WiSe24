package com.spark.matcher;
import org.apache.spark.api.java.JavaRDD;
import org.apache.spark.api.java.JavaSparkContext;
import java.util.*;
import org.apache.log4j.Logger;
import org.apache.log4j.Level;

public class DocumentMatcher {

    static {
        
        Logger.getLogger("org.apache.spark").setLevel(Level.OFF);
        Logger.getLogger("org.apache.spark.storage").setLevel(Level.OFF);
        Logger.getLogger("org.apache.spark.scheduler").setLevel(Level.OFF);
        Logger.getLogger("org.apache.spark.executor").setLevel(Level.OFF);
        Logger.getLogger("org.apache.spark.ui").setLevel(Level.OFF);
        Logger.getLogger("org.apache.spark.SparkContext").setLevel(Level.OFF);
        Logger.getRootLogger().setLevel(Level.OFF);
    }
    public static long[] matchQueries(JavaSparkContext sc, List<Query> queries, String document) {
        try {
            
            if (sc == null || queries == null || document == null) {
                System.err.println("Null input to matchQueries");
                return new long[0];
            }

            
            JavaRDD<Query> queriesRDD = sc.parallelize(queries).cache();
            
            
            List<Long> results = queriesRDD
                .filter(query -> matchDocument(query, document))
                .map(Query::getId)
                .collect();

            
            return results.stream()
                .mapToLong(Long::longValue)
                .toArray();
                
        } catch (Exception e) {
            System.err.println("Error in matchQueries: " + e.getMessage());
            e.printStackTrace();
            return new long[0];
        }
    }

    private static boolean matchDocument(Query query, String document) {
        try {
            if (query == null || document == null) return false;
            
            switch (query.getMatchType()) {
                case 0: return exactMatch(query.getQueryStr(), document);
                case 1: return hammingMatch(query.getQueryStr(), document, query.getMatchDist());
                case 2: return editDistanceMatch(query.getQueryStr(), document, query.getMatchDist());
                default: return false;
            }
        } catch (Exception e) {
            System.err.println("Error in matchDocument: " + e.getMessage());
            return false;
        }
    }
    
    private static boolean exactMatch(String query, String document) {
        document = document.toLowerCase();
        query = query.toLowerCase();
        
        
        Set<String> documentWords = new HashSet<>(Arrays.asList(document.split("\\s+")));
        String[] queryWords = query.split("\\s+");
        
        
        for (String queryWord : queryWords) {
            if (!documentWords.contains(queryWord)) {
                return false;
            }
        }
        return true;
    }
    
    private static boolean hammingMatch(String query, String document, int maxDist) {
        String[] queryWords = query.toLowerCase().split("\\s+");
        Set<String> docWords = new HashSet<>(Arrays.asList(document.toLowerCase().split("\\s+")));
        
        for (String queryWord : queryWords) {
            boolean foundMatch = false;
            for (String docWord : docWords) {
                if (queryWord.length() == docWord.length() && 
                    hammingDistance(queryWord, docWord) <= maxDist) {
                    foundMatch = true;
                    break;
                }
            }
            if (!foundMatch) return false;
        }
        return true;
    }
    
    private static boolean editDistanceMatch(String query, String document, int maxDist) {
        String[] queryWords = query.toLowerCase().split("\\s+");
        Set<String> docWords = new HashSet<>(Arrays.asList(document.toLowerCase().split("\\s+")));
        
        for (String queryWord : queryWords) {
            boolean foundMatch = false;
            for (String docWord : docWords) {
                if (editDistance(queryWord, docWord) <= maxDist) {
                    foundMatch = true;
                    break;
                }
            }
            if (!foundMatch) return false;
        }
        return true;
    }
    
    private static int hammingDistance(String a, String b) {
        if (a.length() != b.length()) return Integer.MAX_VALUE;
        int distance = 0;
        for (int i = 0; i < a.length(); i++) {
            if (a.charAt(i) != b.charAt(i)) distance++;
        }
        return distance;
    }
    
    
    private static int editDistance(String a, String b) {
        int m = a.length(), n = b.length();
        int[][] dp = new int[m + 1][n + 1];
        
        for (int i = 0; i <= m; i++) dp[i][0] = i;
        for (int j = 0; j <= n; j++) dp[0][j] = j;
        
        for (int i = 1; i <= m; i++) {
            for (int j = 1; j <= n; j++) {
                dp[i][j] = Math.min(
                    dp[i - 1][j] + 1,
                    Math.min(dp[i][j - 1] + 1,
                    dp[i - 1][j - 1] + (a.charAt(i-1) == b.charAt(j-1) ? 0 : 1))
                );
            }
        }
        return dp[m][n];
    }
}